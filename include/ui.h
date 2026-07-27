#pragma once

#include "scanner_service.h"

#include <string>

enum class UiScreen {
    None,
    Scanning,
    Found,
    NotFound,
};

class UserInterface {
public:
    void begin();
    void showScanning();
    void showFound(const ScannedDevice& device);
    void showNotFound();

private:
    void prepareScreen(uint16_t accentColor);
    static std::string shortened(const std::string& text, size_t limit);

    UiScreen current_ = UiScreen::None;
    std::string lastDeviceSignature_;
};

