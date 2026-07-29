#pragma once

#include "wedo/connection_model.h"

#include <NimBLEDevice.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>
#include <mutex>
#include <string>

namespace wedo {

class ConnectionService {
public:
    ConnectionService();

    bool begin();
    bool connect(const std::string& name, const std::string& address,
                 uint8_t addressType);
    void reset();
    bool readyForScan() const;
    ConnectionSnapshot snapshot() const;

private:
    class ClientCallbacks final : public NimBLEClientCallbacks {
    public:
        explicit ClientCallbacks(ConnectionService& owner) : owner_(owner) {}
        void onConnect(NimBLEClient* client) override;
        void onConnectFail(NimBLEClient* client, int reason) override;
        void onDisconnect(NimBLEClient* client, int reason) override;

    private:
        ConnectionService& owner_;
    };

    static void discoveryTaskEntry(void* context);
    void discoveryTask();
    void discoverAndSubscribe(NimBLEClient* client);
    void handleAttachedIo(const uint8_t* data, size_t length);
    void handleConnect(NimBLEClient* client);
    void handleConnectFail(int reason);
    void handleDisconnect(int reason);
    void setFailure(const std::string& message, bool disconnectClient);
    static std::string errorWithCode(const char* prefix, int code);

    mutable std::mutex mutex_;
    ConnectionModel model_;
    NimBLEClient* client_ = nullptr;
    NimBLERemoteCharacteristic* attachedIo_ = nullptr;
    TaskHandle_t discoveryTaskHandle_ = nullptr;
    ClientCallbacks callbacks_;
    bool resetRequested_ = false;
    bool shutdownPending_ = false;
};

}  // namespace wedo
