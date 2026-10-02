// transport.cpp - queue + transport task.

#include "app_transport.h"

#include <atomic>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "app_tasks.h"
#include "app_wdt.h"

namespace {

constexpr char TAG[] = "transport";

QueueHandle_t s_queue = nullptr;
const transport_sink_t *s_sinks[TRANSPORT_MAX_SINKS] = {};
std::atomic<size_t> s_nsinks{0};
std::atomic<uint32_t> s_dropped{0};

void transport_task(void *) {
    app_wdt_add();
    app_msg_t msg;
    for (;;) {
        app_wdt_feed();
        if (xQueueReceive(s_queue, &msg, pdMS_TO_TICKS(APP_WDT_WAIT_MS)) != pdTRUE) continue;
        const size_t n = s_nsinks.load();
        for (size_t i = 0; i < n; i++) {
            hal_result_t r = s_sinks[i]->send(&msg);
            if (r != HAL_OK) {
                ESP_LOGW(TAG, "sink %s failed: %s", s_sinks[i]->name, hal_result_name(r));
            }
        }
    }
}

}  // namespace

hal_result_t transport_init(void) {
    s_queue = xQueueCreate(TRANSPORT_QUEUE_LEN, sizeof(app_msg_t));
    if (s_queue == nullptr) return HAL_ERR_NO_MEM;
    BaseType_t ok = xTaskCreatePinnedToCore(transport_task, "transport", APP_STACK_TRANSPORT,
                                            nullptr, APP_PRIO_IO, nullptr, APP_CORE_NET);
    return ok == pdPASS ? HAL_OK : HAL_ERR_NO_MEM;
}

hal_result_t transport_add_sink(const transport_sink_t *sink) {
    if (sink == nullptr || sink->send == nullptr) return HAL_ERR_INVALID_ARG;
    const size_t n = s_nsinks.load();
    if (n >= TRANSPORT_MAX_SINKS) return HAL_ERR_NO_MEM;
    s_sinks[n] = sink;
    s_nsinks.store(n + 1);  // publish after the slot is written
    ESP_LOGI(TAG, "sink added: %s", sink->name);
    return HAL_OK;
}

bool transport_send(const app_msg_t *msg) {
    if (s_queue == nullptr || msg == nullptr) return false;
    if (xQueueSendToBack(s_queue, msg, 0) != pdTRUE) {
        s_dropped++;
        ESP_LOGW(TAG, "queue full, dropped port %u payload", msg->port);
        return false;
    }
    return true;
}

void transport_flush(void) {
    if (s_queue != nullptr) xQueueReset(s_queue);
}

uint32_t transport_queued(void) {
    return s_queue != nullptr ? static_cast<uint32_t>(uxQueueMessagesWaiting(s_queue)) : 0;
}

uint32_t transport_dropped(void) {
    return s_dropped.load();
}
