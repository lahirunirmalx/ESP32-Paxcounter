// payload.h - binary payload encoder.
//
// Byte-compatible with the legacy firmware:
//   ENCODING_PLAIN  - multi-byte fields big-endian
//   ENCODING_PACKED - multi-byte fields little-endian, flags bit-packed
//
// Writes are bounds-checked. On overflow the writer stops writing and
// overflowed() returns true; callers must drop that payload.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app_msg.h"
#include "config_types.h"

struct StatusInfo {
    uint16_t voltage_mv;    // battery voltage, 0 if not measured
    uint64_t uptime_s;
    float cpu_temp_c;       // 0 if no sensor
    uint32_t free_heap;
    uint8_t reset_reason;   // hal_reset_reason_t
    uint32_t restarts;
};

class PayloadWriter {
public:
    explicit PayloadWriter(PayloadEncoding enc) : enc_(enc) {}

    void reset() {
        cursor_ = 0;
        overflow_ = false;
    }

    const uint8_t *data() const { return buf_; }
    size_t size() const { return cursor_; }
    bool overflowed() const { return overflow_; }
    PayloadEncoding encoding() const { return enc_; }

    void add_byte(uint8_t v) { put_u8(v); }
    void add_count(uint16_t v) { put_u16(v); }
    void add_voltage(uint16_t mv) { put_u16(mv); }
    void add_button(uint8_t v) { put_u8(v); }
    void add_time(uint32_t epoch_s) { put_u32(epoch_s); }
    void add_config(const DeviceConfig &cfg);
    void add_status(const StatusInfo &st);

    // Copies the encoded payload into a transport message.
    bool to_msg(uint8_t port, app_msg_t *msg) const;

private:
    void put_u8(uint8_t v);
    void put_u16(uint16_t v) { put_uint(v, 2); }
    void put_u32(uint32_t v) { put_uint(v, 4); }
    void put_u64(uint64_t v) { put_uint(v, 8); }
    void put_uint(uint64_t v, size_t bytes);
    void put_raw(const void *src, size_t len);

    PayloadEncoding enc_;
    uint8_t buf_[APP_PAYLOAD_MAX] = {};
    size_t cursor_ = 0;
    bool overflow_ = false;
};
