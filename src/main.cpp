#include <Arduino.h>
#include <M5Cardputer.h>

#include "scanner_service.h"
#include "ui.h"

namespace {
ScannerService scanner;
UserInterface ui;
bool restartPending = false;

void requestFreshScan(const char* source) {
    Serial.printf("[APP] New scan requested by %s\n", source);
    scanner.stopScan();
    restartPending = true;
    ui.showScanning();
}

void handleKeyboard() {
    if (!M5Cardputer.Keyboard.isChange() ||
        !M5Cardputer.Keyboard.isPressed()) {
        return;
    }

    const Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
    if (keys.del) {
        requestFreshScan("ESC/Backspace");
        return;
    }

    for (const char key : keys.word) {
        if (key == 'r' || key == 'R') {
            requestFreshScan("R key");
            return;
        }
        if (key == static_cast<char>(0x1B)) {
            requestFreshScan("ESC key");
            return;
        }
    }
    // Enter is deliberately ignored: v1 never connects to a hub.
}
}  // namespace

void setup() {
    Serial.begin(115200);

    auto config = M5.config();
    M5Cardputer.begin(config, true);
    ui.begin();

    Serial.println();
    Serial.println("[APP] WeDo Workshop starting");
    Serial.println("[APP] Original Cardputer / BLE scan only");
    Serial.println("[APP] No connection, pairing, or GATT writes");

    scanner.begin();
    scanner.startScan();
}

void loop() {
    M5Cardputer.update();
    handleKeyboard();

    if (restartPending && !scanner.isScanning()) {
        restartPending = !scanner.startScan();
    }

    ScannedDevice wedo;
    if (scanner.bestWedo(wedo)) {
        ui.showFound(wedo);
    } else if (scanner.scanFinished() && !restartPending) {
        ui.showNotFound();
    }

    delay(5);
}
