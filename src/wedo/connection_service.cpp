#include "wedo/connection_service.h"

#include <Arduino.h>

#include <sstream>

namespace wedo {
namespace {
const NimBLEUUID kDeviceServiceUuid("00001523-1212-EFDE-1523-785FEABCD123");
const NimBLEUUID kAttachedIoUuid("00001527-1212-EFDE-1523-785FEABCD123");
constexpr uint32_t kConnectTimeoutMs = 8000;
constexpr uint32_t kDiscoveryTaskStack = 6144;
}  // namespace

ConnectionService::ConnectionService() : callbacks_(*this) {}

bool ConnectionService::begin() {
    if (discoveryTaskHandle_ != nullptr) {
        return true;
    }
    const BaseType_t created =
        xTaskCreate(discoveryTaskEntry, "wedo-gatt", kDiscoveryTaskStack, this,
                    1, &discoveryTaskHandle_);
    if (created != pdPASS) {
        Serial.println("[WEDO] Failed to create the GATT discovery task");
        return false;
    }
    Serial.println("[WEDO] Connection service ready");
    return true;
}

bool ConnectionService::connect(const std::string& name,
                                const std::string& address,
                                uint8_t addressType) {
    NimBLEClient* client = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const ConnectionState state = model_.snapshot().state;
        if (state == ConnectionState::Connecting ||
            state == ConnectionState::DiscoveringModules ||
            state == ConnectionState::Connected ||
            discoveryTaskHandle_ == nullptr) {
            return false;
        }

        resetRequested_ = false;
        shutdownPending_ = false;
        attachedIo_ = nullptr;
        model_.startConnection(name, address);

        if (client_ == nullptr) {
            client_ = NimBLEDevice::createClient(
                NimBLEAddress(address, addressType));
            if (client_ == nullptr) {
                model_.fail("Unable to allocate a BLE client");
                return false;
            }
            client_->setClientCallbacks(&callbacks_, false);
            client_->setConnectTimeout(kConnectTimeoutMs);
            client_->setConnectRetries(1);
        }
        client = client_;
    }

    Serial.printf("[WEDO] Connecting asynchronously to %s\n",
                  address.c_str());
    const NimBLEAddress peerAddress(address, addressType);
    if (!client->connect(peerAddress, true, true, false)) {
        setFailure(errorWithCode("Unable to start BLE connection",
                                 client->getLastError()),
                   false);
        return false;
    }
    return true;
}

void ConnectionService::reset() {
    NimBLEClient* client = nullptr;
    ConnectionState previousState = ConnectionState::Idle;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previousState = model_.snapshot().state;
        resetRequested_ = true;
        shutdownPending_ =
            previousState == ConnectionState::Connecting ||
            previousState == ConnectionState::DiscoveringModules ||
            previousState == ConnectionState::Connected ||
            (client_ != nullptr && client_->isConnected());
        attachedIo_ = nullptr;
        model_.reset();
        client = client_;
    }

    if (client == nullptr) {
        return;
    }
    bool stopRequested = true;
    if (client->isConnected()) {
        Serial.println("[WEDO] Disconnecting before a fresh scan");
        stopRequested = client->disconnect();
    } else if (previousState == ConnectionState::Connecting) {
        stopRequested = client->cancelConnect();
    }
    if (!stopRequested) {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdownPending_ = false;
    }
}

bool ConnectionService::readyForScan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !shutdownPending_;
}

ConnectionSnapshot ConnectionService::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_.snapshot();
}

void ConnectionService::ClientCallbacks::onConnect(NimBLEClient* client) {
    owner_.handleConnect(client);
}

void ConnectionService::ClientCallbacks::onConnectFail(NimBLEClient*,
                                                       int reason) {
    owner_.handleConnectFail(reason);
}

void ConnectionService::ClientCallbacks::onDisconnect(NimBLEClient*,
                                                      int reason) {
    owner_.handleDisconnect(reason);
}

void ConnectionService::discoveryTaskEntry(void* context) {
    static_cast<ConnectionService*>(context)->discoveryTask();
}

void ConnectionService::discoveryTask() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        NimBLEClient* client = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!resetRequested_ &&
                model_.snapshot().state ==
                    ConnectionState::DiscoveringModules) {
                client = client_;
            }
        }
        if (client != nullptr && client->isConnected()) {
            discoverAndSubscribe(client);
        }
    }
}

void ConnectionService::discoverAndSubscribe(NimBLEClient* client) {
    Serial.println("[WEDO] Discovering the official WeDo 2.0 GATT service");
    NimBLERemoteService* service = client->getService(kDeviceServiceUuid);
    if (service == nullptr) {
        setFailure("Official WeDo 2.0 service 1523 was not found", true);
        return;
    }

    NimBLERemoteCharacteristic* characteristic =
        service->getCharacteristic(kAttachedIoUuid);
    if (characteristic == nullptr || !characteristic->canNotify()) {
        setFailure("Attached I/O notification characteristic 1527 is unavailable",
                   true);
        return;
    }

    const bool subscribed = characteristic->subscribe(
        true,
        [this](NimBLERemoteCharacteristic*, uint8_t* data, size_t length,
               bool) { handleAttachedIo(data, length); });
    if (!subscribed) {
        setFailure("Unable to subscribe to attached I/O notifications", true);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resetRequested_ || !client->isConnected()) {
            return;
        }
        attachedIo_ = characteristic;
        model_.moduleDiscoveryFinished();
    }
    Serial.println("[WEDO] Connected; attached I/O notifications enabled");
}

void ConnectionService::handleAttachedIo(const uint8_t* data, size_t length) {
    const ModuleUpdate update = parseAttachedIoNotification(data, length);
    if (!update.valid) {
        Serial.printf("[WEDO] Ignored invalid Attached I/O packet: %s\n",
                      update.error.c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resetRequested_) {
            return;
        }
        model_.applyModuleUpdate(update);
    }

    if (!update.attached) {
        Serial.printf("[WEDO] Module detached (connect ID %u)\n",
                      static_cast<unsigned>(update.connectId));
        return;
    }

    Serial.printf("[WEDO] Module attached: port index=%u, connect ID=%u, "
                  "type=%s (0x%02X), scope=%s\n",
                  static_cast<unsigned>(update.module.hubIndex),
                  static_cast<unsigned>(update.module.connectId),
                  moduleTypeName(update.module.type),
                  static_cast<unsigned>(update.module.type),
                  update.module.isInternal() ? "internal" : "external");
}

void ConnectionService::handleConnect(NimBLEClient* client) {
    bool shouldDisconnect = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resetRequested_) {
            shouldDisconnect = true;
        } else {
            model_.connectionEstablished();
        }
    }
    if (shouldDisconnect) {
        client->disconnect();
        return;
    }
    Serial.printf("[WEDO] BLE connection established with %s\n",
                  client->getPeerAddress().toString().c_str());
    xTaskNotifyGive(discoveryTaskHandle_);
}

void ConnectionService::handleConnectFail(int reason) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resetRequested_) {
            shutdownPending_ = false;
            return;
        }
        model_.fail(errorWithCode("BLE connection failed", reason));
    }
    Serial.printf("[WEDO] BLE connection failed (reason=%d)\n", reason);
}

void ConnectionService::handleDisconnect(int reason) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        attachedIo_ = nullptr;
        if (resetRequested_) {
            shutdownPending_ = false;
            return;
        }
        model_.disconnected(errorWithCode("Hub disconnected", reason));
    }
    Serial.printf("[WEDO] Hub disconnected (reason=%d)\n", reason);
}

void ConnectionService::setFailure(const std::string& message,
                                   bool disconnectClient) {
    NimBLEClient* client = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resetRequested_) {
            return;
        }
        model_.fail(message);
        client = client_;
    }
    Serial.printf("[WEDO] %s\n", message.c_str());
    if (disconnectClient && client != nullptr && client->isConnected()) {
        client->disconnect();
    }
}

std::string ConnectionService::errorWithCode(const char* prefix, int code) {
    std::ostringstream stream;
    stream << prefix << " (code " << code << ')';
    return stream.str();
}

}  // namespace wedo
