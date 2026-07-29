#include "wedo/connection_model.h"

#include <algorithm>

namespace wedo {
namespace {
constexpr size_t kAttachedPacketSize = 12;
constexpr size_t kDetachedPacketSize = 2;
}  // namespace

ModuleUpdate parseAttachedIoNotification(const uint8_t* data, size_t length) {
    ModuleUpdate update;
    if (data == nullptr || length < kDetachedPacketSize) {
        update.error = "Attached I/O notification is shorter than 2 bytes";
        return update;
    }

    update.connectId = data[0];
    update.attached = data[1] != 0;
    if (!update.attached) {
        if (length != kDetachedPacketSize) {
            update.error = "Detached I/O notification must contain 2 bytes";
            return update;
        }
        update.valid = true;
        return update;
    }

    if (length != kAttachedPacketSize) {
        update.error = "Attached I/O notification must contain 12 bytes";
        return update;
    }

    update.module.connectId = data[0];
    update.module.hubIndex = data[2];
    update.module.type = data[3];
    std::copy_n(data + 4, 4, update.module.hardwareRevision.begin());
    std::copy_n(data + 8, 4, update.module.firmwareRevision.begin());
    update.valid = true;
    return update;
}

const char* moduleTypeName(uint8_t type) {
    switch (type) {
        case 0x01:
            return "Motor";
        case 0x14:
            return "Voltage Sensor";
        case 0x15:
            return "Current Sensor";
        case 0x16:
            return "Piezo Speaker";
        case 0x17:
            return "RGB Light";
        case 0x22:
            return "Tilt Sensor";
        case 0x23:
            return "Motion Sensor";
        default:
            return "Unknown Module";
    }
}

void ConnectionModel::startConnection(const std::string& name,
                                      const std::string& address) {
    snapshot_ = {};
    snapshot_.state = ConnectionState::Connecting;
    snapshot_.hubName = name;
    snapshot_.hubAddress = address;
}

void ConnectionModel::connectionEstablished() {
    if (snapshot_.state == ConnectionState::Connecting) {
        snapshot_.state = ConnectionState::DiscoveringModules;
    }
}

void ConnectionModel::moduleDiscoveryFinished() {
    if (snapshot_.state == ConnectionState::DiscoveringModules) {
        snapshot_.state = ConnectionState::Connected;
    }
}

void ConnectionModel::fail(const std::string& error) {
    snapshot_.state = ConnectionState::Failed;
    snapshot_.error = error;
}

void ConnectionModel::disconnected(const std::string& reason) {
    if (snapshot_.state == ConnectionState::Idle ||
        snapshot_.state == ConnectionState::Failed) {
        return;
    }
    snapshot_.state = ConnectionState::Disconnected;
    snapshot_.error = reason;
}

void ConnectionModel::reset() {
    snapshot_ = {};
}

bool ConnectionModel::applyModuleUpdate(const ModuleUpdate& update) {
    if (!update.valid) {
        return false;
    }

    const auto existing = std::find_if(
        snapshot_.modules.begin(), snapshot_.modules.end(),
        [&update](const Module& module) {
            return module.connectId == update.connectId;
        });

    if (!update.attached) {
        if (existing == snapshot_.modules.end()) {
            return false;
        }
        snapshot_.modules.erase(existing);
        return true;
    }

    if (existing == snapshot_.modules.end()) {
        snapshot_.modules.push_back(update.module);
    } else {
        *existing = update.module;
    }
    std::sort(snapshot_.modules.begin(), snapshot_.modules.end(),
              [](const Module& left, const Module& right) {
                  if (left.hubIndex != right.hubIndex) {
                      return left.hubIndex < right.hubIndex;
                  }
                  return left.connectId < right.connectId;
              });
    return true;
}

ConnectionSnapshot ConnectionModel::snapshot() const {
    return snapshot_;
}

}  // namespace wedo
