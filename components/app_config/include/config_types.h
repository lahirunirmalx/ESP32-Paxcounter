// config_types.h - device runtime configuration and its rules.
//
// Pure C++, no ESP-IDF or FreeRTOS includes: compiled into the host unit
// tests as well as the firmware.
//
// Field set and units match the legacy Arduino firmware so remote commands
// and the config payload (port 3) stay wire-compatible.

#pragma once

#include <stdint.h>

#ifndef PROGVERSION
#define PROGVERSION "0.0.0"
#endif

// payloadmask bits
#define PAYLOAD_COUNT_DATA    (1u << 0)
#define PAYLOAD_RESERVED_DATA (1u << 1)
#define PAYLOAD_MEMS_DATA     (1u << 2)
#define PAYLOAD_GPS_DATA      (1u << 3)
#define PAYLOAD_SENSOR1_DATA  (1u << 4)
#define PAYLOAD_SENSOR2_DATA  (1u << 5)
#define PAYLOAD_SENSOR3_DATA  (1u << 6)
#define PAYLOAD_BATT_DATA     (1u << 7)

// WiFi channel map: bit n-1 = channel n
#define CONFIG_WIFI_CHANNELS_ALL 0x1FFFu
// Channels 1-11 are legal in every region (country "01"). A map without any
// of them makes libpax spin forever looking for a usable channel.
#define CONFIG_WIFI_CHANNELS_WORLD 0x07FFu

enum CounterMode : uint8_t {
    COUNTER_CYCLIC = 0,
    COUNTER_CUMULATIVE = 1,
    COUNTER_CYCLIC_CONFIRMED = 2,
};

enum PayloadEncoding : uint8_t {
    ENCODING_PLAIN = 1,   // big-endian, fixed layout
    ENCODING_PACKED = 2,  // little-endian, bit-packed flags
};

struct DeviceConfig {
    char version[10];       // firmware version that wrote this config
    uint8_t loradr;         // 0-15, LoRa datarate
    uint8_t txpower;        // 2-15, LoRa TX power
    uint8_t adrmode;        // 0=disabled, 1=enabled
    uint8_t screensaver;    // 0=disabled, 1=enabled
    uint8_t screenon;       // 0=disabled, 1=enabled
    uint8_t countermode;    // CounterMode
    int16_t rssilimit;      // 0=off, else negative dBm threshold
    uint8_t sendcycle;      // payload send cycle [seconds/2]
    uint16_t sleepcycle;    // sleep after send cycle [seconds/10], 0=off
    uint16_t wakesync;      // wakeup sync window [seconds]
    uint8_t wifichancycle;  // WiFi channel switch interval [seconds/100]
    uint16_t wifichanmap;   // WiFi channel hopping bitmap
    uint8_t blescantime;    // BLE scan duration [seconds], 0=infinite
    uint8_t blescan;        // 0=disabled, 1=enabled
    uint8_t wifiscan;       // 0=disabled, 1=enabled
    uint8_t wifiant;        // 0=internal, 1=external antenna
    uint8_t rgblum;         // RGB LED luminosity 0..100 %
    uint8_t payloadmask;    // PAYLOAD_*_DATA bits
    uint8_t encoding;       // PayloadEncoding
};

// Bumped whenever DeviceConfig changes layout. A stored config with a
// different schema is discarded and replaced by factory defaults.
#define CONFIG_SCHEMA_VERSION 1u

#define CONFIG_SENDCYCLE_MIN 5u     // 10 s
#define CONFIG_RGBLUM_DEFAULT 30u

void config_defaults(DeviceConfig *cfg);

// Clamps out-of-range fields to safe values. Returns the number of fields
// that were changed (0 = config was already valid).
int config_sanitize(DeviceConfig *cfg);

// Report interval in seconds derived from sendcycle.
static inline uint16_t config_send_interval_s(const DeviceConfig *cfg) {
    return (uint16_t)(cfg->sendcycle * 2u);
}
