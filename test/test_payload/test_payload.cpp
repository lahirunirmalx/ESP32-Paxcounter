// Host tests for the payload encoder: byte layout must match the legacy
// firmware so existing network-server decoders keep working.

#include <string.h>
#include <unity.h>

#include "../../components/app_config/config_rules.cpp"
#include "../../components/app_payload/payload.cpp"

void setUp(void) {}
void tearDown(void) {}

static void test_count_packed_is_little_endian(void) {
    PayloadWriter p(ENCODING_PACKED);
    p.add_count(0x0102);
    TEST_ASSERT_EQUAL_UINT32(2, p.size());
    TEST_ASSERT_EQUAL_HEX8(0x02, p.data()[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, p.data()[1]);
}

static void test_count_plain_is_big_endian(void) {
    PayloadWriter p(ENCODING_PLAIN);
    p.add_count(0x0102);
    TEST_ASSERT_EQUAL_HEX8(0x01, p.data()[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, p.data()[1]);
}

static void test_config_packed_layout(void) {
    DeviceConfig c;
    config_defaults(&c);
    c.rssilimit = -80;
    c.countermode = COUNTER_CUMULATIVE;
    PayloadWriter p(ENCODING_PACKED);
    p.add_config(c);
    TEST_ASSERT_EQUAL_UINT32(21, p.size());
    const uint8_t *d = p.data();
    TEST_ASSERT_EQUAL_HEX8(c.loradr, d[0]);
    TEST_ASSERT_EQUAL_HEX8(c.txpower, d[1]);
    TEST_ASSERT_EQUAL_HEX8(0xB0, d[2]);  // -80 = 0xFFB0, little-endian
    TEST_ASSERT_EQUAL_HEX8(0xFF, d[3]);
    TEST_ASSERT_EQUAL_HEX8(c.sendcycle, d[4]);
    // adr=1 screensaver=0 screenon=1 countermode!=0 blescan=0 wifiant=0
    TEST_ASSERT_EQUAL_HEX8(0xB0, d[9]);
    TEST_ASSERT_EQUAL_HEX8(c.payloadmask, d[10]);
    TEST_ASSERT_EQUAL_MEMORY(c.version, d + 11, 10);
}

static void test_config_plain_layout(void) {
    DeviceConfig c;
    config_defaults(&c);
    c.rssilimit = -80;
    PayloadWriter p(ENCODING_PLAIN);
    p.add_config(c);
    TEST_ASSERT_EQUAL_UINT32(27, p.size());
    TEST_ASSERT_EQUAL_HEX8(0xFF, p.data()[6]);  // rssi big-endian
    TEST_ASSERT_EQUAL_HEX8(0xB0, p.data()[7]);
    TEST_ASSERT_EQUAL_HEX8(0x00, p.data()[16]);  // reserved byte
}

static void test_status_layout(void) {
    StatusInfo st = {3700, 0x0102030405060708ull, 42.9f, 0xA0B0C0D0u, 3, 7};
    PayloadWriter p(ENCODING_PACKED);
    p.add_status(st);
    TEST_ASSERT_EQUAL_UINT32(20, p.size());
    TEST_ASSERT_EQUAL_HEX8(0x08, p.data()[2]);   // uptime LSB first
    TEST_ASSERT_EQUAL_HEX8(0x01, p.data()[9]);
    TEST_ASSERT_EQUAL_HEX8(42, p.data()[10]);    // cpu temp truncated
    TEST_ASSERT_EQUAL_HEX8(0xD0, p.data()[11]);
    TEST_ASSERT_EQUAL_HEX8(3, p.data()[15]);
    TEST_ASSERT_EQUAL_HEX8(7, p.data()[16]);
}

static void test_overflow_is_detected_and_blocks_msg(void) {
    PayloadWriter p(ENCODING_PACKED);
    for (int i = 0; i < APP_PAYLOAD_MAX; i++) p.add_byte(0xAA);
    TEST_ASSERT_FALSE(p.overflowed());
    p.add_count(1);
    TEST_ASSERT_TRUE(p.overflowed());
    TEST_ASSERT_EQUAL_UINT32(APP_PAYLOAD_MAX, p.size());
    app_msg_t msg;
    TEST_ASSERT_FALSE(p.to_msg(1, &msg));
}

static void test_to_msg_copies_port_and_bytes(void) {
    PayloadWriter p(ENCODING_PACKED);
    p.add_count(5);
    p.add_count(6);
    app_msg_t msg;
    TEST_ASSERT_TRUE(p.to_msg(1, &msg));
    TEST_ASSERT_EQUAL_UINT8(1, msg.port);
    TEST_ASSERT_EQUAL_UINT8(4, msg.size);
    const uint8_t expect[] = {5, 0, 6, 0};
    TEST_ASSERT_EQUAL_MEMORY(expect, msg.data, 4);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_count_packed_is_little_endian);
    RUN_TEST(test_count_plain_is_big_endian);
    RUN_TEST(test_config_packed_layout);
    RUN_TEST(test_config_plain_layout);
    RUN_TEST(test_status_layout);
    RUN_TEST(test_overflow_is_detected_and_blocks_msg);
    RUN_TEST(test_to_msg_copies_port_and_bytes);
    return UNITY_END();
}
