#include "ui.h"

#include <M5Cardputer.h>

#include <sstream>

namespace {
constexpr uint16_t kBackground = TFT_BLACK;
constexpr uint16_t kText = TFT_WHITE;
constexpr uint16_t kScanningAccent = TFT_CYAN;
constexpr uint16_t kFoundAccent = TFT_GREEN;
constexpr uint16_t kMissingAccent = TFT_ORANGE;
}  // namespace

void UserInterface::begin() {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(100);
    M5Cardputer.Display.setTextWrap(false);
    showScanning();
}

void UserInterface::prepareScreen(uint16_t accentColor) {
    M5Cardputer.Display.fillScreen(kBackground);
    M5Cardputer.Display.fillRect(0, 0, 5, M5Cardputer.Display.height(),
                                 accentColor);
    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextDatum(top_left);
    M5Cardputer.Display.setTextFont(1);
}

void UserInterface::showScanning() {
    if (current_ == UiScreen::Scanning) {
        return;
    }
    current_ = UiScreen::Scanning;
    lastDeviceSignature_.clear();
    prepareScreen(kScanningAccent);

    M5Cardputer.Display.setTextColor(kScanningAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("WeDo Workshop", 12, 8);
    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("Turn on your WeDo 2.0", 12, 42);
    M5Cardputer.Display.drawString("and press the hub button.", 12, 58);
    M5Cardputer.Display.setTextColor(kScanningAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("Scanning...", 12, 88);
}

void UserInterface::showFound(const ScannedDevice& device) {
    std::ostringstream signature;
    signature << device.address << '|' << device.name << '|' << device.rssi;
    if (current_ == UiScreen::Found &&
        lastDeviceSignature_ == signature.str()) {
        return;
    }
    current_ = UiScreen::Found;
    lastDeviceSignature_ = signature.str();
    prepareScreen(kFoundAccent);

    M5Cardputer.Display.setTextColor(kFoundAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("WeDo 2.0 found", 12, 5);

    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    const std::string name =
        shortened(device.name.empty() ? "unknown" : device.name, 29);
    M5Cardputer.Display.drawString(("Name: " + name).c_str(), 12, 32);
    M5Cardputer.Display.drawString(("MAC: " + device.address).c_str(), 12, 47);
    M5Cardputer.Display.drawString(
        ("RSSI: " + std::to_string(device.rssi) + " dBm").c_str(), 12, 62);
    M5Cardputer.Display.drawString("Type: WeDo 2.0 Hub", 12, 77);

    M5Cardputer.Display.setTextColor(kFoundAccent, kBackground);
    M5Cardputer.Display.drawString("R: scan again", 12, 103);
    M5Cardputer.Display.drawString("ESC/Bksp: reset", 12, 118);
}

void UserInterface::showNotFound() {
    if (current_ == UiScreen::NotFound) {
        return;
    }
    current_ = UiScreen::NotFound;
    lastDeviceSignature_.clear();
    prepareScreen(kMissingAccent);

    M5Cardputer.Display.setTextColor(kMissingAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("No WeDo 2.0", 12, 4);
    M5Cardputer.Display.drawString("found", 12, 23);

    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("Check that:", 12, 49);
    M5Cardputer.Display.drawString("- the hub is powered on;", 12, 63);
    M5Cardputer.Display.drawString("- its button was pressed;", 12, 76);
    M5Cardputer.Display.drawString("- no other device is connected.", 12, 89);
    M5Cardputer.Display.setTextColor(kMissingAccent, kBackground);
    M5Cardputer.Display.drawString("R: try again", 12, 116);
}

std::string UserInterface::shortened(const std::string& text, size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    if (limit <= 3) {
        return text.substr(0, limit);
    }
    return text.substr(0, limit - 3) + "...";
}
