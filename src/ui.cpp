#include "ui.h"

#include <M5Cardputer.h>

#include <iomanip>
#include <sstream>

namespace {
constexpr uint16_t kBackground = TFT_BLACK;
constexpr uint16_t kText = TFT_WHITE;
constexpr uint16_t kScanningAccent = TFT_CYAN;
constexpr uint16_t kFoundAccent = TFT_GREEN;
constexpr uint16_t kConnectingAccent = TFT_YELLOW;
constexpr uint16_t kErrorAccent = TFT_RED;
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
    lastSignature_.clear();
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

void UserInterface::showConnecting(
    const wedo::ConnectionSnapshot& connection) {
    std::ostringstream signature;
    signature << static_cast<int>(connection.state) << '|'
              << connection.hubAddress;
    if (current_ == UiScreen::Connecting && lastSignature_ == signature.str()) {
        return;
    }
    current_ = UiScreen::Connecting;
    lastSignature_ = signature.str();
    prepareScreen(kConnectingAccent);

    M5Cardputer.Display.setTextColor(kConnectingAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("WeDo 2.0 found", 12, 5);

    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    const std::string name =
        shortened(connection.hubName.empty() ? "unknown" : connection.hubName,
                  29);
    M5Cardputer.Display.drawString(("Name: " + name).c_str(), 12, 32);
    M5Cardputer.Display.drawString(
        ("MAC: " + connection.hubAddress).c_str(), 12, 47);

    M5Cardputer.Display.setTextColor(kConnectingAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    const char* status = connection.state ==
                                 wedo::ConnectionState::DiscoveringModules
                             ? "Reading modules..."
                             : "Connecting...";
    M5Cardputer.Display.drawString(status, 12, 75);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString("R or Backspace: cancel", 12, 117);
}

void UserInterface::showConnected(
    const wedo::ConnectionSnapshot& connection) {
    std::ostringstream signature;
    signature << connection.hubAddress;
    for (const auto& module : connection.modules) {
        signature << '|' << static_cast<unsigned>(module.connectId) << ':'
                  << static_cast<unsigned>(module.hubIndex) << ':'
                  << static_cast<unsigned>(module.type);
    }
    if (current_ == UiScreen::Connected && lastSignature_ == signature.str()) {
        return;
    }
    current_ = UiScreen::Connected;
    lastSignature_ = signature.str();
    prepareScreen(kFoundAccent);

    M5Cardputer.Display.setTextColor(kFoundAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("WeDo connected", 12, 4);
    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString(
        ("MAC: " + connection.hubAddress).c_str(), 12, 28);
    M5Cardputer.Display.drawString("Connected modules:", 12, 45);

    size_t externalCount = 0;
    int y = 61;
    for (const auto& module : connection.modules) {
        if (module.isInternal()) {
            continue;
        }
        std::ostringstream line;
        line << "Port " << static_cast<unsigned>(module.hubIndex + 1) << ": "
             << wedo::moduleTypeName(module.type);
        if (std::string(wedo::moduleTypeName(module.type)) ==
            "Unknown Module") {
            line << " (0x" << std::uppercase << std::hex << std::setw(2)
                 << std::setfill('0') << static_cast<unsigned>(module.type)
                 << ')';
        }
        M5Cardputer.Display.drawString(
            shortened(line.str(), 35).c_str(), 12, y);
        y += 16;
        ++externalCount;
    }

    if (externalCount == 0) {
        M5Cardputer.Display.drawString("No external modules detected.", 12, y);
        M5Cardputer.Display.drawString("Attach one; this list updates live.",
                                       12, y + 16);
    }

    M5Cardputer.Display.setTextColor(kFoundAccent, kBackground);
    M5Cardputer.Display.drawString("R or Backspace: scan again", 12, 119);
}

void UserInterface::showConnectionError(const std::string& error) {
    const std::string signature = shortened(error, 80);
    if (current_ == UiScreen::ConnectionError && lastSignature_ == signature) {
        return;
    }
    current_ = UiScreen::ConnectionError;
    lastSignature_ = signature;
    prepareScreen(kErrorAccent);

    M5Cardputer.Display.setTextColor(kErrorAccent, kBackground);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.drawString("Connection failed", 12, 4);
    M5Cardputer.Display.setTextColor(kText, kBackground);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.drawString(shortened(error, 36).c_str(), 12, 35);
    if (error.size() > 36) {
        M5Cardputer.Display.drawString(
            shortened(error.substr(36), 36).c_str(), 12, 51);
    }
    M5Cardputer.Display.drawString("Make sure the hub is awake and", 12, 78);
    M5Cardputer.Display.drawString("not connected to another device.", 12, 93);
    M5Cardputer.Display.setTextColor(kErrorAccent, kBackground);
    M5Cardputer.Display.drawString("R or Backspace: try again", 12, 118);
}

void UserInterface::showNotFound() {
    if (current_ == UiScreen::NotFound) {
        return;
    }
    current_ = UiScreen::NotFound;
    lastSignature_.clear();
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
