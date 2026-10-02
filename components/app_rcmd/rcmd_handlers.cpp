// rcmd_handlers.cpp - legacy-compatible opcode table.
//
// Config changes apply to RAM immediately; send 0x21 (save config) to
// persist them. Opcodes for hardware this build does not have (LoRa radio,
// GPS, BME, battery ADC) still update the stored setting where one exists,
// and log that the feature is not available.

#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "app_pax.h"
#include "app_ports.h"
#include "app_transport.h"
#include "hal_sys.h"
#include "payload.h"
#include "rcmd_parser.h"

namespace {

constexpr char TAG[] = "rcmd";

enum TimeSource : uint8_t { TIME_GPS, TIME_RTC, TIME_LORA, TIME_UNSYNCED, TIME_SET };
TimeSource s_time_source = TIME_UNSYNCED;

// Set by handlers that change sniffing parameters. The rcmd task restarts
// libpax once after the whole command batch (handlers only run on that task).
bool s_pax_restart_pending = false;

PayloadEncoding encoding() {
    return static_cast<PayloadEncoding>(config_get().encoding);
}

void send(const PayloadWriter &p, uint8_t port) {
    app_msg_t msg;
    if (p.to_msg(port, &msg)) transport_send(&msg);
}

void restart_after_log(void) {
    vTaskDelay(pdMS_TO_TICKS(200));  // let the log line drain
    hal_sys_restart();
}

void not_available(const char *what) {
    ESP_LOGW(TAG, "%s is not available on this board", what);
}

// Field setters: each copies the value and writes it under the config lock.
template <typename T>
struct FieldWrite {
    T DeviceConfig::*field;
    T value;
};

template <typename T>
void put_field(T DeviceConfig::*field, T value) {
    FieldWrite<T> w{field, value};
    config_update([](DeviceConfig *c, void *p) {
        auto *fw = static_cast<FieldWrite<T> *>(p);
        c->*(fw->field) = fw->value;
    }, &w);
}

// Same, then schedules a libpax restart so the change takes effect.
template <typename T>
void put_field_restart(T DeviceConfig::*field, T value) {
    put_field(field, value);
    s_pax_restart_pending = true;
}

uint8_t flag(uint8_t v) { return v ? 1 : 0; }

void set_mask_bit(uint8_t bit, bool on) {
    uint8_t args[2] = {bit, static_cast<uint8_t>(on ? 1 : 0)};
    config_update([](DeviceConfig *c, void *v) {
        const uint8_t *a = static_cast<const uint8_t *>(v);
        c->payloadmask = a[1] ? static_cast<uint8_t>(c->payloadmask | a[0])
                              : static_cast<uint8_t>(c->payloadmask & ~a[0]);
    }, args);
}

// --- handlers -------------------------------------------------------------

void set_rssi(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "set RSSI limit to -%u", a[0]);
    put_field_restart(&DeviceConfig::rssilimit, static_cast<int16_t>(-a[0]));
}

void set_countmode(const uint8_t *a, void *) {
    if (a[0] > COUNTER_CYCLIC_CONFIRMED) {
        ESP_LOGW(TAG, "set counter mode: invalid value %u", a[0]);
        return;
    }
    ESP_LOGI(TAG, "set counter mode to %u", a[0]);
    put_field_restart(&DeviceConfig::countermode, a[0]);
}

void set_gps(const uint8_t *a, void *) { set_mask_bit(PAYLOAD_GPS_DATA, a[0]); }
void set_bme(const uint8_t *a, void *) { set_mask_bit(PAYLOAD_MEMS_DATA, a[0]); }
void set_batt(const uint8_t *a, void *) { set_mask_bit(PAYLOAD_BATT_DATA, a[0]); }

void set_display(const uint8_t *a, void *) { put_field(&DeviceConfig::screenon, flag(a[0])); }
void set_screensaver(const uint8_t *a, void *) { put_field(&DeviceConfig::screensaver, flag(a[0])); }

void set_loradr(const uint8_t *a, void *) {
    put_field(&DeviceConfig::loradr, a[0]);
    not_available("LoRa");
}

void set_lorapower(const uint8_t *a, void *) {
    put_field(&DeviceConfig::txpower, a[0]);
    not_available("LoRa");
}

void set_loraadr(const uint8_t *a, void *) {
    put_field(&DeviceConfig::adrmode, flag(a[0]));
    not_available("LoRa");
}

void set_reset(const uint8_t *a, void *) {
    switch (a[0]) {
        case 0:
            ESP_LOGW(TAG, "restart (cold)");
            restart_after_log();
            break;
        case 1:
            ESP_LOGI(TAG, "reset MAC counter: deprecated, ignored");
            break;
        case 2:
            ESP_LOGW(TAG, "factory reset and restart");
            config_factory_reset();
            restart_after_log();
            break;
        case 3:
            ESP_LOGI(TAG, "flush send queue");
            transport_flush();
            break;
        case 4:
            ESP_LOGW(TAG, "restart (warm)");
            restart_after_log();
            break;
        case 8:
        case 9:
            not_available("OTA / maintenance mode");
            break;
        default:
            ESP_LOGW(TAG, "reset: invalid parameter %u", a[0]);
    }
}

void set_sendcycle(const uint8_t *a, void *) {
    if (a[0] < CONFIG_SENDCYCLE_MIN) {
        ESP_LOGW(TAG, "send cycle %u s too short, min %u s", a[0] * 2, CONFIG_SENDCYCLE_MIN * 2);
        return;
    }
    ESP_LOGI(TAG, "set send cycle to %u s", a[0] * 2);
    put_field_restart(&DeviceConfig::sendcycle, a[0]);
}

void set_wifichancycle(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "set WiFi channel switch interval to %u0 ms", a[0]);
    put_field_restart(&DeviceConfig::wifichancycle, a[0]);
}

void set_blescantime(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "set BLE scan time to %u s", a[0]);
    put_field_restart(&DeviceConfig::blescantime, a[0]);
}

void set_wakesync(const uint8_t *a, void *) { put_field(&DeviceConfig::wakesync, rcmd_be16(a)); }

void set_blescan(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "BLE scan %s", a[0] ? "on" : "off");
    put_field_restart(&DeviceConfig::blescan, flag(a[0]));
}

void set_wifiscan(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "WiFi scan %s", a[0] ? "on" : "off");
    put_field_restart(&DeviceConfig::wifiscan, flag(a[0]));
}

void set_wifiant(const uint8_t *a, void *) {
    put_field(&DeviceConfig::wifiant, flag(a[0]));
    not_available("antenna switch");
}

void set_rgblum(const uint8_t *a, void *) { put_field(&DeviceConfig::rgblum, a[0]); }

void set_wifichanmap(const uint8_t *a, void *) {
    const uint16_t v = rcmd_be16(a);
    ESP_LOGI(TAG, "set WiFi channel map to 0x%04x", v);
    put_field_restart(&DeviceConfig::wifichanmap, v);
}

void set_sensor(const uint8_t *a, void *) {
    if (a[0] < 1 || a[0] > 3) {
        ESP_LOGW(TAG, "set sensor: invalid sensor number %u", a[0]);
        return;
    }
    set_mask_bit(static_cast<uint8_t>(PAYLOAD_SENSOR1_DATA << (a[0] - 1)), a[1]);
}

void set_payloadmask(const uint8_t *a, void *) {
    ESP_LOGI(TAG, "set payload mask to 0x%02x", a[0]);
    put_field(&DeviceConfig::payloadmask, a[0]);
}

void set_sleepcycle(const uint8_t *a, void *) {
    const uint16_t v = rcmd_be16(a);
    ESP_LOGI(TAG, "set sleep cycle to %u s (deep sleep not implemented yet)", v * 10);
    put_field(&DeviceConfig::sleepcycle, v);
}

void set_flush(const uint8_t *, void *) {
    // No-op: opens a receive window on LoRaWAN class A nodes.
}

void set_loadconfig(const uint8_t *, void *) {
    ESP_LOGI(TAG, "load config: %s", hal_result_name(config_reload()));
    s_pax_restart_pending = true;
}

void set_saveconfig(const uint8_t *, void *) {
    config_save();
}

void get_config(const uint8_t *, void *) {
    const DeviceConfig cfg = config_get();
    PayloadWriter p(static_cast<PayloadEncoding>(cfg.encoding));
    p.add_config(cfg);
    send(p, APP_PORT_CONFIG);
}

void get_status(const uint8_t *, void *) {
    StatusInfo st = {};
    st.voltage_mv = 0;
    st.uptime_s = hal_sys_uptime_ms() / 1000u;
    hal_sys_cpu_temp(&st.cpu_temp_c);
    st.free_heap = hal_sys_free_heap();
    st.reset_reason = static_cast<uint8_t>(hal_sys_reset_reason());
    st.restarts = config_restart_count();
    PayloadWriter p(encoding());
    p.add_status(st);
    send(p, APP_PORT_STATUS);
}

void get_batt(const uint8_t *, void *) { not_available("battery measurement"); }
void get_gps(const uint8_t *, void *) { not_available("GPS"); }
void get_bme(const uint8_t *, void *) { not_available("BME sensor"); }
void set_timesync(const uint8_t *, void *) { not_available("network time sync"); }

void get_time(const uint8_t *, void *) {
    PayloadWriter p(encoding());
    p.add_time(static_cast<uint32_t>(time(nullptr)));
    const uint8_t synced = (s_time_source == TIME_SET) ? 1 : 0;
    p.add_byte(static_cast<uint8_t>((synced << 4) | s_time_source));
    send(p, APP_PORT_TIME);
}

void set_time(const uint8_t *a, void *) {
    struct timeval tv = {};
    tv.tv_sec = static_cast<time_t>(rcmd_be32(a));
    settimeofday(&tv, nullptr);
    s_time_source = TIME_SET;
    ESP_LOGI(TAG, "time set to %lu", static_cast<unsigned long>(tv.tv_sec));
}

}  // namespace

// Opcode table: {opcode, argument bytes, handler}. Same numbering as the
// legacy firmware.
extern const RcmdEntry g_rcmd_table[] = {
    {0x01, 1, set_rssi},          {0x02, 1, set_countmode},
    {0x03, 1, set_gps},           {0x04, 1, set_display},
    {0x05, 1, set_loradr},        {0x06, 1, set_lorapower},
    {0x07, 1, set_loraadr},       {0x08, 1, set_screensaver},
    {0x09, 1, set_reset},         {0x0a, 1, set_sendcycle},
    {0x0b, 1, set_wifichancycle}, {0x0c, 1, set_blescantime},
    {0x0d, 2, set_wakesync},      {0x0e, 1, set_blescan},
    {0x0f, 1, set_wifiant},       {0x10, 1, set_rgblum},
    {0x11, 2, set_wifichanmap},   {0x13, 2, set_sensor},
    {0x14, 1, set_payloadmask},   {0x15, 1, set_bme},
    {0x16, 1, set_batt},          {0x17, 1, set_wifiscan},
    {0x18, 0, set_flush},         {0x19, 2, set_sleepcycle},
    {0x20, 0, set_loadconfig},    {0x21, 0, set_saveconfig},
    {0x80, 0, get_config},        {0x81, 0, get_status},
    {0x83, 0, get_batt},          {0x84, 0, get_gps},
    {0x85, 0, get_bme},           {0x86, 0, get_time},
    {0x87, 0, set_timesync},      {0x88, 4, set_time},
    {0x99, 0, set_flush},
};
extern const size_t g_rcmd_table_len = sizeof(g_rcmd_table) / sizeof(g_rcmd_table[0]);

// Returns true (once) if the last batch changed a sniffing parameter.
bool rcmd_take_pax_restart(void) {
    const bool pending = s_pax_restart_pending;
    s_pax_restart_pending = false;
    return pending;
}
