#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wedo {

enum class ConnectionState {
    Idle,
    Connecting,
    DiscoveringModules,
    Connected,
    Failed,
    Disconnected,
};

struct Module {
    uint8_t connectId = 0;
    uint8_t hubIndex = 0;
    uint8_t type = 0;
    std::array<uint8_t, 4> hardwareRevision{};
    std::array<uint8_t, 4> firmwareRevision{};

    bool isInternal() const { return hubIndex >= 50; }
};

struct ModuleUpdate {
    bool valid = false;
    bool attached = false;
    uint8_t connectId = 0;
    Module module;
    std::string error;
};

struct ConnectionSnapshot {
    ConnectionState state = ConnectionState::Idle;
    std::string hubName;
    std::string hubAddress;
    std::vector<Module> modules;
    std::string error;
};

ModuleUpdate parseAttachedIoNotification(const uint8_t* data, size_t length);
const char* moduleTypeName(uint8_t type);

class ConnectionModel {
public:
    void startConnection(const std::string& name, const std::string& address);
    void connectionEstablished();
    void moduleDiscoveryFinished();
    void fail(const std::string& error);
    void disconnected(const std::string& reason);
    void reset();
    bool applyModuleUpdate(const ModuleUpdate& update);
    ConnectionSnapshot snapshot() const;

private:
    ConnectionSnapshot snapshot_;
};

}  // namespace wedo
