// Host tests for the remote command parser and hex line decoder.

#include <unity.h>

#include "../../components/app_rcmd/rcmd_parser.cpp"

namespace {
int g_calls;
uint8_t g_last_op;
uint8_t g_last_arg;

void h_a(const uint8_t *a, void *) { g_calls++; g_last_op = 0x0a; g_last_arg = a[0]; }
void h_save(const uint8_t *, void *) { g_calls++; g_last_op = 0x21; }
void h_two(const uint8_t *a, void *) { g_calls++; g_last_op = 0x11; g_last_arg = a[1]; }

const RcmdEntry kTable[] = {{0x0a, 1, h_a}, {0x21, 0, h_save}, {0x11, 2, h_two}};
constexpr size_t kTableLen = sizeof(kTable) / sizeof(kTable[0]);
}  // namespace

void setUp(void) {
    g_calls = 0;
    g_last_op = 0;
    g_last_arg = 0;
}
void tearDown(void) {}

static void test_single_command(void) {
    const uint8_t cmd[] = {0x0a, 0x0f};
    RcmdResult r = rcmd_execute(cmd, sizeof(cmd), kTable, kTableLen, nullptr);
    TEST_ASSERT_EQUAL(RCMD_OK, r.status);
    TEST_ASSERT_EQUAL(1, r.executed);
    TEST_ASSERT_EQUAL_HEX8(0x0f, g_last_arg);
}

static void test_concatenated_commands_run_in_order(void) {
    const uint8_t cmd[] = {0x0a, 0x1e, 0x11, 0x00, 0x07, 0x21};
    RcmdResult r = rcmd_execute(cmd, sizeof(cmd), kTable, kTableLen, nullptr);
    TEST_ASSERT_EQUAL(RCMD_OK, r.status);
    TEST_ASSERT_EQUAL(3, r.executed);
    TEST_ASSERT_EQUAL(3, g_calls);
    TEST_ASSERT_EQUAL_HEX8(0x21, g_last_op);
}

static void test_unknown_opcode_stops_after_earlier_commands(void) {
    const uint8_t cmd[] = {0x21, 0x55, 0x21};
    RcmdResult r = rcmd_execute(cmd, sizeof(cmd), kTable, kTableLen, nullptr);
    TEST_ASSERT_EQUAL(RCMD_UNKNOWN_OPCODE, r.status);
    TEST_ASSERT_EQUAL_HEX8(0x55, r.bad_opcode);
    TEST_ASSERT_EQUAL(1, r.executed);
}

static void test_truncated_args_are_not_run(void) {
    const uint8_t cmd[] = {0x11, 0x00};
    RcmdResult r = rcmd_execute(cmd, sizeof(cmd), kTable, kTableLen, nullptr);
    TEST_ASSERT_EQUAL(RCMD_MISSING_ARGS, r.status);
    TEST_ASSERT_EQUAL(0, g_calls);
}

static void test_empty(void) {
    RcmdResult r = rcmd_execute(nullptr, 0, kTable, kTableLen, nullptr);
    TEST_ASSERT_EQUAL(RCMD_EMPTY, r.status);
}

static void test_hex_decode_formats(void) {
    uint8_t out[8];
    TEST_ASSERT_EQUAL_UINT32(2, rcmd_hex_decode("0a 1e", out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8(0x0a, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x1e, out[1]);
    TEST_ASSERT_EQUAL_UINT32(2, rcmd_hex_decode("0A1E\r", out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8(0x1e, out[1]);
    TEST_ASSERT_EQUAL_UINT32(3, rcmd_hex_decode("0x0a,0x1e 21", out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8(0x21, out[2]);
    TEST_ASSERT_EQUAL_UINT32(1, rcmd_hex_decode("00", out, sizeof(out)));
    TEST_ASSERT_EQUAL_HEX8(0x00, out[0]);
}

static void test_hex_decode_rejects_bad_input(void) {
    uint8_t out[2];
    TEST_ASSERT_EQUAL_UINT32(0, rcmd_hex_decode("0g", out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT32(0, rcmd_hex_decode("abc", out, sizeof(out)));     // odd digits
    TEST_ASSERT_EQUAL_UINT32(0, rcmd_hex_decode("a b", out, sizeof(out)));     // split byte
    TEST_ASSERT_EQUAL_UINT32(0, rcmd_hex_decode("010203", out, sizeof(out)));  // over cap
    TEST_ASSERT_EQUAL_UINT32(0, rcmd_hex_decode("", out, sizeof(out)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_single_command);
    RUN_TEST(test_concatenated_commands_run_in_order);
    RUN_TEST(test_unknown_opcode_stops_after_earlier_commands);
    RUN_TEST(test_truncated_args_are_not_run);
    RUN_TEST(test_empty);
    RUN_TEST(test_hex_decode_formats);
    RUN_TEST(test_hex_decode_rejects_bad_input);
    return UNITY_END();
}
