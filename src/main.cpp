#include <Arduino.h>
#include <M5Cardputer.h>

#include "scanner_service.h"
#include "ui.h"
#include "wedo/connection_service.h"

namespace {
ScannerService scanner;
wedo::ConnectionService connection;
UserInterface ui;
bool restartPending = false;
bool appReady = false;

void requestFreshScan(const char* source) {
    Serial.printf("[APP] New scan requested by %s\n", source);
    connection.reset();
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
}
}  // namespace

void setup() {
    Serial.begin(115200);

    auto config = M5.config();
    M5Cardputer.begin(config, true);
    ui.begin();

    Serial.println();
    Serial.println("[APP] WeDo Workshop starting");
    Serial.println("[APP] Original Cardputer / WeDo 2.0 connection");

    scanner.begin();
    if (!connection.begin()) {
        ui.showConnectionError("Unable to start connection service");
        return;
    }
    scanner.startScan();
    appReady = true;
}

void loop() {
    M5Cardputer.update();
    handleKeyboard();

    if (!appReady) {
        delay(20);
        return;
    }

    if (restartPending && connection.readyForScan() &&
        !scanner.isScanning()) {
        restartPending = !scanner.startScan();
    }

    ScannedDevice hub;
    if (!restartPending && scanner.bestWedo(hub)) {
        const auto state = connection.snapshot().state;
        if (state == wedo::ConnectionState::Idle) {
            scanner.stopScan();
            connection.connect(hub.name, hub.address, hub.addressType);
        }
    }

    const wedo::ConnectionSnapshot snapshot = connection.snapshot();
    switch (snapshot.state) {
        case wedo::ConnectionState::Connecting:
        case wedo::ConnectionState::DiscoveringModules:
            ui.showConnecting(snapshot);
            break;
        case wedo::ConnectionState::Connected:
            ui.showConnected(snapshot);
            break;
        case wedo::ConnectionState::Failed:
        case wedo::ConnectionState::Disconnected:
            ui.showConnectionError(snapshot.error);
            break;
        case wedo::ConnectionState::Idle:
            if (scanner.scanFinished() && !restartPending) {
                ui.showNotFound();
            }
            break;
    }

    delay(5);
}
