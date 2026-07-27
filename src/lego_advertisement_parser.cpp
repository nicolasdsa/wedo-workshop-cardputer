#include "lego_advertisement_parser.h"

#include <iomanip>
#include <sstream>

namespace {
constexpr uint16_t kLegoManufacturerId = 0x0397;

std::string hex16(uint16_t value) {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(4)
           << std::setfill('0') << value;
    return stream.str();
}
}  // namespace

LegoAdvertisementInfo parseLegoAdvertisement(
    const std::string& manufacturerData,
    bool hasLegoService,
    const std::string& advertisedName,
    bool hasWedo20LegacyService) {
    LegoAdvertisementInfo info;
    info.hasLegoService = hasLegoService;
    info.hasWedo20LegacyService = hasWedo20LegacyService;
    info.hasManufacturerData = !manufacturerData.empty();
    info.advertisedName = advertisedName;

    if (hasLegoService) {
        info.isLego = true;
        info.criteria.emplace_back("advertised LEGO LWP 3.x service 1623");
    }

    // WeDo 2.0 predates the 1623 LWP 3.x profile. Its official Communication
    // Software Developer Kit identifies the Smart Hub by the dedicated legacy
    // service 00001523-1212-EFDE-1523-785FEABCD123. This full 128-bit UUID is
    // specific evidence for WeDo 2.0 even when Manufacturer Data is absent.
    if (hasWedo20LegacyService) {
        info.isLego = true;
        info.isWedo20 = true;
        info.hubType = LegoHubType::Wedo20;
        info.criteria.emplace_back("advertised official WeDo 2.0 service 1523");
    }

    if (manufacturerData.empty()) {
        info.criteria.emplace_back("Manufacturer Data absent");
        if (!advertisedName.empty()) {
            info.criteria.emplace_back("BLE name is not used as evidence");
        }
        return info;
    }

    if (manufacturerData.size() < 2) {
        info.parsingError =
            "truncated Manufacturer Data: missing two-byte Manufacturer ID";
        info.criteria.emplace_back("invalid Manufacturer Data");
        return info;
    }

    // Bluetooth Company Identifiers inside Manufacturer Specific Data are
    // serialized least-significant byte first: LEGO 0x0397 => 97 03.
    info.manufacturerId =
        static_cast<uint8_t>(manufacturerData[0]) |
        (static_cast<uint16_t>(static_cast<uint8_t>(manufacturerData[1])) << 8);

    if (info.manufacturerId != kLegoManufacturerId) {
        info.criteria.emplace_back("Manufacturer ID " + hex16(info.manufacturerId) +
                                   " is not assigned to LEGO (expected 0x0397)");
        if (hasLegoService) {
            info.parsingError =
                "conflicting evidence: LEGO service with a different Manufacturer ID";
        }
        return info;
    }

    info.isLego = true;
    info.hasLegoManufacturerId = true;
    info.criteria.emplace_back("LEGO Manufacturer ID 0x0397 (bytes 97 03)");

    // getManufacturerData() removed AD length/type. Payload layout is:
    // [0..1] company ID, [2] button, [3] SSS DDDDD system/device.
    if (manufacturerData.size() < 4) {
        if (!hasWedo20LegacyService) {
            info.hubType = LegoHubType::OtherLego;
        }
        info.parsingError =
            "truncated LEGO Manufacturer Data: System Type/Device Number field absent";
        info.criteria.emplace_back("hub type unavailable");
        return info;
    }

    const uint8_t systemAndDevice =
        static_cast<uint8_t>(manufacturerData[3]);
    info.systemType = static_cast<uint8_t>((systemAndDevice >> 5) & 0x07);
    info.deviceNumber = static_cast<uint8_t>(systemAndDevice & 0x1F);

    if (info.systemType == 0 && info.deviceNumber == 0) {
        info.isWedo20 = true;
        info.hubType = LegoHubType::Wedo20;
        info.criteria.emplace_back("System Type 000 + Device Number 00000");
    } else if (hasWedo20LegacyService) {
        info.parsingError =
            "conflicting evidence: WeDo 2.0 service 1523 with a different LWP type";
        info.criteria.emplace_back(
            "different LWP type; dedicated service 1523 preserves WeDo 2.0 identification");
    } else {
        info.hubType = LegoHubType::OtherLego;
        std::ostringstream criterion;
        criterion << "different LEGO hub type (system="
                  << static_cast<unsigned>(info.systemType)
                  << ", device=" << static_cast<unsigned>(info.deviceNumber) << ')';
        info.criteria.push_back(criterion.str());
    }

    return info;
}

const char* legoHubTypeName(LegoHubType type) {
    switch (type) {
        case LegoHubType::Wedo20:
            return "WeDo 2.0 Hub";
        case LegoHubType::OtherLego:
            return "Other LEGO hub";
        default:
            return "Unknown";
    }
}

std::string joinIdentificationCriteria(const LegoAdvertisementInfo& info) {
    if (info.criteria.empty()) {
        return "no official LEGO evidence";
    }

    std::ostringstream stream;
    for (size_t index = 0; index < info.criteria.size(); ++index) {
        if (index != 0) {
            stream << "; ";
        }
        stream << info.criteria[index];
    }
    return stream.str();
}
