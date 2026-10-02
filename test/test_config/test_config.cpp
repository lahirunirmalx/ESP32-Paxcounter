// Host tests for config defaults and validation.

#include <unity.h>

#include "../../components/app_config/config_rules.cpp"

void setUp(void) {}
void tearDown(void) {}

static void test_defaults_are_valid(void) {
    DeviceConfig c;
    config_defaults(&c);
    TEST_ASSERT_EQUAL(0, config_sanitize(&c));
    TEST_ASSERT_EQUAL_UINT16(60, config_send_interval_s(&c));
    TEST_ASSERT_EQUAL_UINT8(1, c.wifiscan);
    TEST_ASSERT_EQUAL_UINT8(0, c.blescan);
    TEST_ASSERT_EQUAL_UINT8(ENCODING_PACKED, c.encoding);
    TEST_ASSERT_EQUAL_STRING_LEN(PROGVERSION, c.version, 5);
}

static void test_sanitize_fixes_out_of_range(void) {
    DeviceConfig c;
    config_defaults(&c);
    c.sendcycle = 1;
    c.countermode = 9;
    c.rssilimit = 70;
    c.rgblum = 200;
    c.wifichanmap = 0;
    c.encoding = 7;
    TEST_ASSERT_EQUAL(6, config_sanitize(&c));
    TEST_ASSERT_EQUAL_UINT8(CONFIG_SENDCYCLE_MIN, c.sendcycle);
    TEST_ASSERT_EQUAL_UINT8(COUNTER_CYCLIC, c.countermode);
    TEST_ASSERT_EQUAL_INT16(-70, c.rssilimit);
    TEST_ASSERT_EQUAL_UINT8(CONFIG_RGBLUM_DEFAULT, c.rgblum);
    TEST_ASSERT_EQUAL_HEX16(CONFIG_WIFI_CHANNELS_ALL, c.wifichanmap);
    TEST_ASSERT_EQUAL_UINT8(ENCODING_PACKED, c.encoding);
}

static void test_sanitize_masks_unknown_channels(void) {
    DeviceConfig c;
    config_defaults(&c);
    c.wifichanmap = 0xE001;  // channel 1 + bits beyond channel 13
    TEST_ASSERT_EQUAL(1, config_sanitize(&c));
    TEST_ASSERT_EQUAL_HEX16(0x0001, c.wifichanmap);
}

static void test_sanitize_rejects_map_without_world_channels(void) {
    DeviceConfig c;
    config_defaults(&c);
    c.wifichanmap = 0x1800;  // channels 12 + 13 only: unusable with country "01"
    TEST_ASSERT_EQUAL(1, config_sanitize(&c));
    TEST_ASSERT_EQUAL_HEX16(CONFIG_WIFI_CHANNELS_ALL, c.wifichanmap);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid);
    RUN_TEST(test_sanitize_fixes_out_of_range);
    RUN_TEST(test_sanitize_masks_unknown_channels);
    RUN_TEST(test_sanitize_rejects_map_without_world_channels);
    return UNITY_END();
}
