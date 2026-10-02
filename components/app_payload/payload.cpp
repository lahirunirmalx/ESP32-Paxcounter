// payload.cpp - payload encoder implementation.

#include "payload.h"

#include <string.h>

void PayloadWriter::put_u8(uint8_t v) {
    if (overflow_ || cursor_ >= sizeof(buf_)) {
        overflow_ = true;
        return;
    }
    buf_[cursor_++] = v;
}

void PayloadWriter::put_uint(uint64_t v, size_t bytes) {
    if (overflow_ || cursor_ + bytes > sizeof(buf_)) {
        overflow_ = true;
        return;
    }
    for (size_t i = 0; i < bytes; i++) {
        size_t shift = (enc_ == ENCODING_PLAIN) ? (bytes - 1 - i) * 8 : i * 8;
        buf_[cursor_++] = static_cast<uint8_t>((v >> shift) & 0xFFu);
    }
}

void PayloadWriter::put_raw(const void *src, size_t len) {
    if (overflow_ || cursor_ + len > sizeof(buf_)) {
        overflow_ = true;
        return;
    }
    memcpy(buf_ + cursor_, src, len);
    cursor_ += len;
}

void PayloadWriter::add_config(const DeviceConfig &c) {
    const uint16_t rssi = static_cast<uint16_t>(c.rssilimit);
    if (enc_ == ENCODING_PLAIN) {
        put_u8(c.loradr);
        put_u8(c.txpower);
        put_u8(c.adrmode);
        put_u8(c.screensaver);
        put_u8(c.screenon);
        put_u8(c.countermode);
        put_u16(rssi);
        put_u8(c.sendcycle);
        put_u8(c.wifichancycle);
        put_u8(c.blescantime);
        put_u8(c.blescan);
        put_u8(c.wifiant);
        put_u16(c.sleepcycle);
        put_u8(c.payloadmask);
        put_u8(0);  // reserved
    } else {
        put_u8(c.loradr);
        put_u8(c.txpower);
        put_u16(rssi);
        put_u8(c.sendcycle);
        put_u8(c.wifichancycle);
        put_u8(c.blescantime);
        put_u16(c.sleepcycle);
        // MSB first: adr, screensaver, screenon, countermode, blescan, wifiant
        uint8_t bits = 0;
        bits = static_cast<uint8_t>(bits | ((c.adrmode ? 1u : 0u) << 7));
        bits = static_cast<uint8_t>(bits | ((c.screensaver ? 1u : 0u) << 6));
        bits = static_cast<uint8_t>(bits | ((c.screenon ? 1u : 0u) << 5));
        bits = static_cast<uint8_t>(bits | ((c.countermode ? 1u : 0u) << 4));
        bits = static_cast<uint8_t>(bits | ((c.blescan ? 1u : 0u) << 3));
        bits = static_cast<uint8_t>(bits | ((c.wifiant ? 1u : 0u) << 2));
        put_u8(bits);
        put_u8(c.payloadmask);
    }
    put_raw(c.version, sizeof(c.version));
}

void PayloadWriter::add_status(const StatusInfo &st) {
    put_u16(st.voltage_mv);
    put_u64(st.uptime_s);
    put_u8(static_cast<uint8_t>(static_cast<int>(st.cpu_temp_c)));
    put_u32(st.free_heap);
    put_u8(st.reset_reason);
    put_u32(st.restarts);
}

bool PayloadWriter::to_msg(uint8_t port, app_msg_t *msg) const {
    if (msg == nullptr || overflow_) return false;
    msg->port = port;
    msg->size = static_cast<uint8_t>(cursor_);
    memcpy(msg->data, buf_, cursor_);
    return true;
}
