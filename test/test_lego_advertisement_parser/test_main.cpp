#include <unity.h>

#include <string>

#include "lego_advertisement_parser.h"

namespace {
// Spec-derived bytes:
//   97 03 = LEGO Company ID 0x0397 in BLE little-endian order.
//   payload byte 3 = SSS DDDDD (System Type and Device Number).
// The remaining button/capability/network/status/option values are illustrative.
const std::string kValidWedo =
    std::string("\x97\x03\x00\x00\x05\x00\x41\x00", 8);

void test_valid_wedo_advertisement() {
    const auto info = parseLegoAdvertisement(kValidWedo, true, "LPF2 Smart Hub");
    TEST_ASSERT_TRUE(info.isLego);
    TEST_ASSERT_TRUE(info.isWedo20);
    TEST_ASSERT_EQUAL(LegoHubType::Wedo20, info.hubType);
    TEST_ASSERT_EQUAL_UINT8(0, info.systemType);
    TEST_ASSERT_EQUAL_UINT8(0, info.deviceNumber);
    TEST_ASSERT_TRUE(info.parsingError.empty());
}

void test_other_lego_hub_type() {
    // 0x40 = System 010 (LEGO System), Device 00000 (Boost Hub).
    const std::string boost("\x97\x03\x00\x40\x05\x00\x41\x00", 8);
    const auto info = parseLegoAdvertisement(boost, true, "Move Hub");
    TEST_ASSERT_TRUE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
    TEST_ASSERT_EQUAL(LegoHubType::OtherLego, info.hubType);
}

void test_different_manufacturer_id() {
    const std::string other("\x4C\x00\x00\x00", 4);
    const auto info = parseLegoAdvertisement(other, false, "");
    TEST_ASSERT_FALSE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
}

void test_empty_data() {
    const auto info = parseLegoAdvertisement("", false, "");
    TEST_ASSERT_FALSE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
    TEST_ASSERT_TRUE(info.parsingError.empty());
}

void test_truncated_lego_data() {
    const std::string truncated("\x97\x03\x00", 3);
    const auto info = parseLegoAdvertisement(truncated, false, "");
    TEST_ASSERT_TRUE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
    TEST_ASSERT_FALSE(info.parsingError.empty());
}

void test_lego_service_without_manufacturer_data() {
    const auto info = parseLegoAdvertisement("", true, "");
    TEST_ASSERT_TRUE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
    TEST_ASSERT_TRUE(info.hasLegoService);
}

void test_manufacturer_bytes_in_wrong_order() {
    const std::string wrongOrder("\x03\x97\x00\x00", 4);
    const auto info = parseLegoAdvertisement(wrongOrder, false, "");
    TEST_ASSERT_FALSE(info.hasLegoManufacturerId);
    TEST_ASSERT_FALSE(info.isWedo20);
}

void test_lego_name_is_not_evidence() {
    const auto info =
        parseLegoAdvertisement("", false, "LEGO Education WeDo 2.0");
    TEST_ASSERT_FALSE(info.isLego);
    TEST_ASSERT_FALSE(info.isWedo20);
}

void test_real_wedo_legacy_service_without_manufacturer_data() {
    // Real-device regression: the original WeDo 2.0 advertises the dedicated
    // full 128-bit 1523 service from the official WeDo 2.0 SDK and may omit
    // Manufacturer Data entirely.
    const auto info = parseLegoAdvertisement("", false, "", true);
    TEST_ASSERT_TRUE(info.isLego);
    TEST_ASSERT_TRUE(info.isWedo20);
    TEST_ASSERT_TRUE(info.hasWedo20LegacyService);
    TEST_ASSERT_EQUAL(LegoHubType::Wedo20, info.hubType);
    TEST_ASSERT_TRUE(info.parsingError.empty());
}
}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_wedo_advertisement);
    RUN_TEST(test_other_lego_hub_type);
    RUN_TEST(test_different_manufacturer_id);
    RUN_TEST(test_empty_data);
    RUN_TEST(test_truncated_lego_data);
    RUN_TEST(test_lego_service_without_manufacturer_data);
    RUN_TEST(test_manufacturer_bytes_in_wrong_order);
    RUN_TEST(test_lego_name_is_not_evidence);
    RUN_TEST(test_real_wedo_legacy_service_without_manufacturer_data);
    return UNITY_END();
}
