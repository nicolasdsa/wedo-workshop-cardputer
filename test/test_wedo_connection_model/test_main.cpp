#include <unity.h>

#include "wedo/connection_model.h"

namespace {
const uint8_t kTiltAttached[] = {
    0x01, 0x01, 0x00, 0x22, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x10,
};

void test_attached_io_packet() {
    const auto update =
        wedo::parseAttachedIoNotification(kTiltAttached, sizeof(kTiltAttached));
    TEST_ASSERT_TRUE(update.valid);
    TEST_ASSERT_TRUE(update.attached);
    TEST_ASSERT_EQUAL_UINT8(1, update.connectId);
    TEST_ASSERT_EQUAL_UINT8(0, update.module.hubIndex);
    TEST_ASSERT_EQUAL_UINT8(0x22, update.module.type);
    TEST_ASSERT_EQUAL_STRING("Tilt Sensor",
                             wedo::moduleTypeName(update.module.type));
    TEST_ASSERT_FALSE(update.module.isInternal());
}

void test_detached_io_packet() {
    const uint8_t packet[] = {0x02, 0x00};
    const auto update =
        wedo::parseAttachedIoNotification(packet, sizeof(packet));
    TEST_ASSERT_TRUE(update.valid);
    TEST_ASSERT_FALSE(update.attached);
    TEST_ASSERT_EQUAL_UINT8(2, update.connectId);
}

void test_invalid_attached_packet_length() {
    const uint8_t packet[] = {0x01, 0x01, 0x00, 0x22};
    const auto update =
        wedo::parseAttachedIoNotification(packet, sizeof(packet));
    TEST_ASSERT_FALSE(update.valid);
    TEST_ASSERT_FALSE(update.error.empty());
}

void test_module_lifecycle_and_sorting() {
    wedo::ConnectionModel model;
    model.startConnection("Hub", "AA:BB:CC:DD:EE:FF");
    model.connectionEstablished();

    const uint8_t motorPortTwo[] = {
        0x02, 0x01, 0x01, 0x01, 0, 0, 0, 1, 0, 0, 0, 1,
    };
    model.applyModuleUpdate(wedo::parseAttachedIoNotification(
        motorPortTwo, sizeof(motorPortTwo)));
    model.applyModuleUpdate(wedo::parseAttachedIoNotification(
        kTiltAttached, sizeof(kTiltAttached)));
    model.moduleDiscoveryFinished();

    auto snapshot = model.snapshot();
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Connected, snapshot.state);
    TEST_ASSERT_EQUAL_UINT32(2, snapshot.modules.size());
    TEST_ASSERT_EQUAL_UINT8(0, snapshot.modules[0].hubIndex);
    TEST_ASSERT_EQUAL_UINT8(1, snapshot.modules[1].hubIndex);

    const uint8_t detached[] = {0x01, 0x00};
    TEST_ASSERT_TRUE(model.applyModuleUpdate(
        wedo::parseAttachedIoNotification(detached, sizeof(detached))));
    snapshot = model.snapshot();
    TEST_ASSERT_EQUAL_UINT32(1, snapshot.modules.size());
    TEST_ASSERT_EQUAL_UINT8(2, snapshot.modules[0].connectId);
}

void test_connection_state_transitions() {
    wedo::ConnectionModel model;
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Idle, model.snapshot().state);
    model.startConnection("Hub", "00:11:22:33:44:55");
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Connecting,
                      model.snapshot().state);
    model.connectionEstablished();
    TEST_ASSERT_EQUAL(wedo::ConnectionState::DiscoveringModules,
                      model.snapshot().state);
    model.moduleDiscoveryFinished();
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Connected,
                      model.snapshot().state);
    model.disconnected("link lost");
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Disconnected,
                      model.snapshot().state);
    TEST_ASSERT_EQUAL_STRING("link lost", model.snapshot().error.c_str());
    model.reset();
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Idle, model.snapshot().state);
}

void test_failure_is_preserved_after_disconnect_callback() {
    wedo::ConnectionModel model;
    model.startConnection("Hub", "00:11:22:33:44:55");
    model.fail("service missing");
    model.disconnected("disconnect callback");
    TEST_ASSERT_EQUAL(wedo::ConnectionState::Failed, model.snapshot().state);
    TEST_ASSERT_EQUAL_STRING("service missing", model.snapshot().error.c_str());
}
}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_attached_io_packet);
    RUN_TEST(test_detached_io_packet);
    RUN_TEST(test_invalid_attached_packet_length);
    RUN_TEST(test_module_lifecycle_and_sorting);
    RUN_TEST(test_connection_state_transitions);
    RUN_TEST(test_failure_is_preserved_after_disconnect_callback);
    return UNITY_END();
}
