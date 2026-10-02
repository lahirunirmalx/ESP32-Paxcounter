// pax.cpp - libpax wrapper and pax report task.

#include "app_pax.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "libpax_api.h"

#include "app_config.h"
#include "app_ports.h"
#include "app_tasks.h"
#include "app_transport.h"
#include "app_wdt.h"
#include "hal_sys.h"
#include "payload.h"

namespace {

constexpr char TAG[] = "pax";

// libpax writes here right before calling report_cb, from its timer task.
count_payload_t s_libpax_count;

QueueHandle_t s_mailbox = nullptr;     // length 1, holds the latest pax_count_t
SemaphoreHandle_t s_ctl_lock = nullptr; // serializes libpax start/stop
SemaphoreHandle_t s_last_lock = nullptr;
pax_count_t s_last = {};

class Lock {
public:
    explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m_); }
    Lock(const Lock &) = delete;
    Lock &operator=(const Lock &) = delete;

private:
    SemaphoreHandle_t m_;
};

// Runs in the FreeRTOS timer task. Must not block.
void report_cb(void) {
    pax_count_t c = {s_libpax_count.pax, s_libpax_count.wifi_count, s_libpax_count.ble_count,
                     hal_sys_uptime_ms()};
    xQueueOverwrite(s_mailbox, &c);
}

libpax_config_t make_libpax_config(const DeviceConfig &cfg) {
    libpax_config_t lc;
    libpax_default_config(&lc);
    lc.wificounter = cfg.wifiscan;
    lc.blecounter = cfg.blescan;
    lc.wifi_channel_map = cfg.wifichanmap;
    lc.wifi_channel_switch_interval = cfg.wifichancycle;
    // wifichancycle 0 = no hopping: stay on channel 1 (legacy behaviour)
    if (cfg.wifichancycle == 0) lc.wifi_channel_map = LIBPAX_WIFI_CHANNEL_1;
    lc.wifi_rssi_threshold = cfg.rssilimit;
    lc.ble_rssi_threshold = cfg.rssilimit;
    lc.blescantime = cfg.blescantime;
    lc.blescanwindow = 80;
    lc.blescaninterval = 80;
    return lc;
}

// Caller holds s_ctl_lock.
hal_result_t start_locked(void) {
    const DeviceConfig cfg = config_get();
    libpax_config_t lc = make_libpax_config(cfg);
    if (libpax_update_config(&lc) != 0) {
        ESP_LOGE(TAG, "libpax rejected config");
        return HAL_ERR_INVALID_ARG;
    }
    if (libpax_counter_init(report_cb, &s_libpax_count, config_send_interval_s(&cfg),
                            cfg.countermode) != 0 ||
        libpax_counter_start() != 0) {
        ESP_LOGE(TAG, "libpax start failed");
        return HAL_ERR_INTERNAL;
    }
    ESP_LOGI(TAG, "counting: wifi=%u ble=%u every %us rssi>=%d mode=%u", cfg.wifiscan,
             cfg.blescan, config_send_interval_s(&cfg), cfg.rssilimit, cfg.countermode);
    return HAL_OK;
}

void send_count(const pax_count_t &c) {
    {
        Lock lock(s_last_lock);
        s_last = c;
    }
    const DeviceConfig cfg = config_get();
    ESP_LOGI(TAG, "pax=%lu wifi=%lu ble=%lu", (unsigned long)c.pax, (unsigned long)c.wifi,
             (unsigned long)c.ble);
    if (!(cfg.payloadmask & PAYLOAD_COUNT_DATA)) return;

    PayloadWriter p(static_cast<PayloadEncoding>(cfg.encoding));
    p.add_count(static_cast<uint16_t>(c.wifi > 0xFFFF ? 0xFFFF : c.wifi));
    if (cfg.blescan) p.add_count(static_cast<uint16_t>(c.ble > 0xFFFF ? 0xFFFF : c.ble));
    app_msg_t msg;
    if (p.to_msg(APP_PORT_COUNTER, &msg)) transport_send(&msg);
}

void pax_report_task(void *) {
    app_wdt_add();
    pax_count_t c;
    for (;;) {
        app_wdt_feed();
        if (xQueueReceive(s_mailbox, &c, pdMS_TO_TICKS(APP_WDT_WAIT_MS)) == pdTRUE) send_count(c);
    }
}

}  // namespace

hal_result_t pax_start(void) {
    s_mailbox = xQueueCreate(1, sizeof(pax_count_t));
    s_ctl_lock = xSemaphoreCreateMutex();
    s_last_lock = xSemaphoreCreateMutex();
    if (!s_mailbox || !s_ctl_lock || !s_last_lock) return HAL_ERR_NO_MEM;

    if (xTaskCreatePinnedToCore(pax_report_task, "pax", APP_STACK_PAX, nullptr, APP_PRIO_DATA,
                                nullptr, APP_CORE_APP) != pdPASS) {
        return HAL_ERR_NO_MEM;
    }
    Lock lock(s_ctl_lock);
    return start_locked();
}

hal_result_t pax_restart(void) {
    Lock lock(s_ctl_lock);
    libpax_counter_stop();
    return start_locked();
}

void pax_report_now(void) {
    count_payload_t live;
    libpax_counter_count(&live);
    send_count({live.pax, live.wifi_count, live.ble_count, hal_sys_uptime_ms()});
}

pax_count_t pax_last(void) {
    Lock lock(s_last_lock);
    return s_last;
}
