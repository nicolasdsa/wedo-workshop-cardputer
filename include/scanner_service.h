#pragma once

#include "lego_advertisement_parser.h"

#include <NimBLEDevice.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct ScannedDevice {
    std::string name;
    std::string address;
    uint8_t addressType = 0;
    int rssi = 0;
    std::vector<std::string> serviceUuids;
    std::string manufacturerData;
    std::string manufacturerDataHex;
    LegoAdvertisementInfo lego;
};

class ScannerService {
public:
    static constexpr uint32_t kScanDurationMs = 15000;

    ScannerService();

    void begin();
    bool startScan();
    void stopScan();
    bool isScanning() const;
    bool scanFinished() const;
    size_t deviceCount() const;
    bool bestWedo(ScannedDevice& output) const;

private:
    class Callbacks final : public NimBLEScanCallbacks {
    public:
        explicit Callbacks(ScannerService& owner) : owner_(owner) {}
        void onResult(const NimBLEAdvertisedDevice* device) override;
        void onScanEnd(const NimBLEScanResults& results, int reason) override;

    private:
        ScannerService& owner_;
    };

    void handleResult(const NimBLEAdvertisedDevice& device);
    void handleScanEnd(int reason);

    NimBLEScan* scan_ = nullptr;
    Callbacks callbacks_;
    mutable std::mutex mutex_;
    std::vector<ScannedDevice> devices_;
    bool scanning_ = false;
    bool scanFinished_ = false;
};

std::string bytesToHex(const std::string& bytes);

