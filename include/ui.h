#pragma once

#include "wedo/connection_model.h"

#include <string>

enum class UiScreen {
    None,
    Scanning,
    Connecting,
    Connected,
    ConnectionError,
    NotFound,
};

class UserInterface {
public:
    void begin();
    void showScanning();
    void showConnecting(const wedo::ConnectionSnapshot& connection);
    void showConnected(const wedo::ConnectionSnapshot& connection);
    void showConnectionError(const std::string& error);
    void showNotFound();

private:
    void prepareScreen(uint16_t accentColor);
    static std::string shortened(const std::string& text, size_t limit);

    UiScreen current_ = UiScreen::None;
    std::string lastSignature_;
};
