// config_rules.cpp - factory defaults and validation. No platform includes.

#include <string.h>

#include "config_types.h"

void config_defaults(DeviceConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    strncpy(cfg->version, PROGVERSION, sizeof(cfg->version) - 1);

    cfg->loradr = 5;
    cfg->txpower = 14;
    cfg->adrmode = 1;
    cfg->screensaver = 0;
    cfg->screenon = 1;
    cfg->countermode = COUNTER_CYCLIC;
    cfg->rssilimit = 0;
    cfg->sendcycle = 30;      // 60 s
    cfg->sleepcycle = 0;
    cfg->wakesync = 300;
    cfg->wifichancycle = 50;  // 0.5 s
    cfg->wifichanmap = CONFIG_WIFI_CHANNELS_ALL;
    cfg->blescantime = 0;     // infinite
    cfg->blescan = 0;     // BLE off by default, as in the legacy firmware
    cfg->wifiscan = 1;
    cfg->wifiant = 0;
    cfg->rgblum = CONFIG_RGBLUM_DEFAULT;
    cfg->payloadmask = PAYLOAD_COUNT_DATA | PAYLOAD_MEMS_DATA | PAYLOAD_GPS_DATA |
                       PAYLOAD_SENSOR1_DATA | PAYLOAD_SENSOR2_DATA | PAYLOAD_SENSOR3_DATA;
    cfg->encoding = ENCODING_PACKED;
}

int config_sanitize(DeviceConfig *cfg) {
    int fixed = 0;
    if (cfg->countermode > COUNTER_CYCLIC_CONFIRMED) {
        cfg->countermode = COUNTER_CYCLIC;
        fixed++;
    }
    if (cfg->sendcycle < CONFIG_SENDCYCLE_MIN) {
        cfg->sendcycle = CONFIG_SENDCYCLE_MIN;
        fixed++;
    }
    if (cfg->rssilimit > 0) {
        cfg->rssilimit = (int16_t)-cfg->rssilimit;
        fixed++;
    }
    if (cfg->rgblum > 100) {
        cfg->rgblum = CONFIG_RGBLUM_DEFAULT;
        fixed++;
    }
    if ((cfg->wifichanmap & CONFIG_WIFI_CHANNELS_WORLD) == 0) {
        cfg->wifichanmap = CONFIG_WIFI_CHANNELS_ALL;
        fixed++;
    } else if (cfg->wifichanmap & ~CONFIG_WIFI_CHANNELS_ALL) {
        cfg->wifichanmap &= CONFIG_WIFI_CHANNELS_ALL;
        fixed++;
    }
    if (cfg->encoding != ENCODING_PLAIN && cfg->encoding != ENCODING_PACKED) {
        cfg->encoding = ENCODING_PACKED;
        fixed++;
    }
    if (cfg->blescan > 1) { cfg->blescan = 1; fixed++; }
    if (cfg->wifiscan > 1) { cfg->wifiscan = 1; fixed++; }
    if (cfg->adrmode > 1) { cfg->adrmode = 1; fixed++; }
    return fixed;
}
