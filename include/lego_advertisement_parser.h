#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class LegoHubType {
    Unknown,
    Wedo20,
    OtherLego,
};

struct LegoAdvertisementInfo {
    bool isLego = false;
    bool isWedo20 = false;
    LegoHubType hubType = LegoHubType::Unknown;
    bool hasLegoService = false;
    bool hasWedo20LegacyService = false;
    bool hasManufacturerData = false;
    bool hasLegoManufacturerId = false;
    uint16_t manufacturerId = 0;
    uint8_t systemType = 0xFF;
    uint8_t deviceNumber = 0xFF;
    std::string advertisedName;
    std::vector<std::string> criteria;
    std::string parsingError;
};

// manufacturerData is the value returned by NimBLE-Arduino's
// getManufacturerData(): it starts with the two company-ID bytes and does not
// contain the enclosing AD length or AD type (0xFF).
LegoAdvertisementInfo parseLegoAdvertisement(
    const std::string& manufacturerData,
    bool hasLegoService,
    const std::string& advertisedName,
    bool hasWedo20LegacyService = false);

const char* legoHubTypeName(LegoHubType type);
std::string joinIdentificationCriteria(const LegoAdvertisementInfo& info);
