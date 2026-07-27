#include "scanner_service.h"

#include <Arduino.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
const NimBLEUUID kLwp3LegoServiceUuid("00001623-1212-EFDE-1623-785FEABCD123");
const NimBLEUUID kWedo20LegacyServiceUuid("00001523-1212-EFDE-1523-785FEABCD123");

const char* addressTypeName(uint8_t type) {
    switch (type) {
        case 0:
            return "public";
        case 1:
            return "random";
        case 2:
            return "public identity";
        case 3:
            return "random identity";
        default:
            return "unknown";
    }
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}
}  // namespace

ScannerService::ScannerService() : callbacks_(*this) {}

void ScannerService::begin() {
    NimBLEDevice::init("");
    scan_ = NimBLEDevice::getScan();
    scan_->setScanCallbacks(&callbacks_, true);
    scan_->setActiveScan(true);
    scan_->setInterval(100);
    scan_->setWindow(80);
    scan_->setDuplicateFilter(0);
    scan_->setMaxResults(0);
    Serial.println("[BLE] NimBLE initialized; no connection will be created");
}

bool ScannerService::startScan() {
    if (scan_ == nullptr || scan_->isScanning()) {
        Serial.println("[BLE] Scan not started: scanner unavailable or already active");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        devices_.clear();
        scanning_ = true;
        scanFinished_ = false;
    }
    scan_->clearResults();

    const bool started = scan_->start(kScanDurationMs, false, false);
    if (!started) {
        std::lock_guard<std::mutex> lock(mutex_);
        scanning_ = false;
        scanFinished_ = true;
        Serial.println("[BLE] Failed to start scan");
        return false;
    }

    Serial.printf("[BLE] Scan started for %lu ms\n",
                  static_cast<unsigned long>(kScanDurationMs));
    return true;
}

void ScannerService::stopScan() {
    if (scan_ != nullptr && scan_->isScanning()) {
        Serial.println("[BLE] Requesting scan stop");
        scan_->stop();
    }
}

bool ScannerService::isScanning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scanning_;
}

bool ScannerService::scanFinished() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scanFinished_;
}

size_t ScannerService::deviceCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return devices_.size();
}

bool ScannerService::bestWedo(ScannedDevice& output) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = std::max_element(
        devices_.begin(), devices_.end(),
        [](const ScannedDevice& left, const ScannedDevice& right) {
            if (left.lego.isWedo20 != right.lego.isWedo20) {
                return !left.lego.isWedo20;
            }
            return left.rssi < right.rssi;
        });
    if (found == devices_.end() || !found->lego.isWedo20) {
        return false;
    }
    output = *found;
    return true;
}

void ScannerService::Callbacks::onResult(
    const NimBLEAdvertisedDevice* device) {
    if (device != nullptr) {
        owner_.handleResult(*device);
    }
}

void ScannerService::Callbacks::onScanEnd(
    const NimBLEScanResults&, int reason) {
    owner_.handleScanEnd(reason);
}

void ScannerService::handleResult(const NimBLEAdvertisedDevice& device) {
    ScannedDevice incoming;
    incoming.name = device.haveName() ? device.getName() : "";
    incoming.address = device.getAddress().toString();
    incoming.addressType = device.getAddressType();
    incoming.rssi = device.getRSSI();

    for (uint8_t index = 0; index < device.getServiceUUIDCount(); ++index) {
        incoming.serviceUuids.push_back(device.getServiceUUID(index).toString());
    }

    const bool hasLegoService =
        device.isAdvertisingService(kLwp3LegoServiceUuid);
    const bool hasWedo20LegacyService =
        device.isAdvertisingService(kWedo20LegacyServiceUuid);
    LegoAdvertisementInfo strongest =
        parseLegoAdvertisement("", hasLegoService, incoming.name,
                               hasWedo20LegacyService);

    for (uint8_t index = 0; index < device.getManufacturerDataCount(); ++index) {
        const std::string data = device.getManufacturerData(index);
        const LegoAdvertisementInfo parsed =
            parseLegoAdvertisement(data, hasLegoService, incoming.name,
                                   hasWedo20LegacyService);
        if (incoming.manufacturerDataHex.empty()) {
            incoming.manufacturerDataHex = bytesToHex(data);
        } else {
            incoming.manufacturerDataHex += " | " + bytesToHex(data);
        }
        if (incoming.manufacturerData.empty() || parsed.hasLegoManufacturerId) {
            incoming.manufacturerData = data;
        }
        if (parsed.isWedo20 ||
            (!strongest.hasLegoManufacturerId && parsed.hasLegoManufacturerId)) {
            strongest = parsed;
        }
    }
    incoming.lego = strongest;

    bool shouldLog = false;
    ScannedDevice snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto existing = std::find_if(
            devices_.begin(), devices_.end(),
            [&incoming](const ScannedDevice& item) {
                return item.address == incoming.address;
            });

        if (existing == devices_.end()) {
            devices_.push_back(incoming);
            snapshot = devices_.back();
            shouldLog = true;
        } else {
            existing->rssi = incoming.rssi;
            existing->addressType = incoming.addressType;
            if (!incoming.name.empty()) {
                existing->name = incoming.name;
            }
            for (const auto& uuid : incoming.serviceUuids) {
                if (!contains(existing->serviceUuids, uuid)) {
                    existing->serviceUuids.push_back(uuid);
                }
            }
            const bool strongerEvidence =
                incoming.lego.isWedo20 ||
                (!existing->lego.hasLegoManufacturerId &&
                 incoming.lego.hasLegoManufacturerId) ||
                (!existing->lego.hasLegoService && incoming.lego.hasLegoService) ||
                (!existing->lego.hasWedo20LegacyService &&
                 incoming.lego.hasWedo20LegacyService);
            if (!incoming.manufacturerData.empty()) {
                existing->manufacturerData = incoming.manufacturerData;
                existing->manufacturerDataHex = incoming.manufacturerDataHex;
            }
            if (strongerEvidence) {
                existing->lego = incoming.lego;
                shouldLog = true;
            }
            snapshot = *existing;
        }
    }

    if (!shouldLog) {
        return;
    }

    Serial.println("[BLE] --- device ---");
    Serial.printf("[BLE] Name: %s\n",
                  snapshot.name.empty() ? "<empty>" : snapshot.name.c_str());
    Serial.printf("[BLE] Address: %s (%s)\n", snapshot.address.c_str(),
                  addressTypeName(snapshot.addressType));
    Serial.printf("[BLE] RSSI: %d dBm\n", snapshot.rssi);
    Serial.print("[BLE] Advertised UUIDs: ");
    if (snapshot.serviceUuids.empty()) {
        Serial.println("<none>");
    } else {
        for (size_t index = 0; index < snapshot.serviceUuids.size(); ++index) {
            Serial.printf("%s%s", index == 0 ? "" : ", ",
                          snapshot.serviceUuids[index].c_str());
        }
        Serial.println();
    }
    Serial.printf("[BLE] Manufacturer Data: %s\n",
                  snapshot.manufacturerDataHex.empty()
                      ? "<absent>"
                      : snapshot.manufacturerDataHex.c_str());
    Serial.printf("[BLE] Classification: LEGO=%s, WeDo2=%s, type=%s\n",
                  snapshot.lego.isLego ? "yes" : "no",
                  snapshot.lego.isWedo20 ? "yes" : "no",
                  legoHubTypeName(snapshot.lego.hubType));
    const std::string criteria = joinIdentificationCriteria(snapshot.lego);
    Serial.printf("[BLE] Reason: %s\n", criteria.c_str());
    if (!snapshot.lego.parsingError.empty()) {
        Serial.printf("[BLE] Parsing error: %s\n",
                      snapshot.lego.parsingError.c_str());
    }
}

void ScannerService::handleScanEnd(int reason) {
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        scanning_ = false;
        scanFinished_ = true;
        count = devices_.size();
    }
    Serial.printf("[BLE] Scan finished (reason=%d); %u unique devices\n",
                  reason, static_cast<unsigned>(count));
}

std::string bytesToHex(const std::string& bytes) {
    if (bytes.empty()) {
        return "";
    }
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (size_t index = 0; index < bytes.size(); ++index) {
        if (index != 0) {
            stream << ' ';
        }
        stream << std::setw(2)
               << static_cast<unsigned>(static_cast<uint8_t>(bytes[index]));
    }
    return stream.str();
}
