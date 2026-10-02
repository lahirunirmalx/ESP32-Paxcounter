// button.cpp - interrupt-driven, debounced button task.

#include "app_button.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "app_pax.h"
#include "app_ports.h"
#include "app_tasks.h"
#include "app_transport.h"
#include "app_wdt.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_sys.h"
#include "payload.h"

namespace {

constexpr char TAG[] = "button";
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kLongPressMs = 1000;
constexpr uint32_t kPollMs = 10;

TaskHandle_t s_task = nullptr;

// Interrupt context: wake the task, nothing else.
void on_edge(uint8_t, void *) {
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    portYIELD_FROM_ISR(woken);
}

bool pressed(void) {
    bool level = false;
    hal_gpio_get(BOARD_BUTTON_PIN, &level);
    return BOARD_BUTTON_ACTIVE_LOW ? !level : level;
}

// Returns the level once it has been stable for kDebounceMs.
bool debounced(void) {
    bool last = pressed();
    uint32_t stable = 0;
    while (stable < kDebounceMs) {
        vTaskDelay(pdMS_TO_TICKS(kPollMs));
        const bool now = pressed();
        stable = (now == last) ? stable + kPollMs : 0;
        last = now;
    }
    return last;
}

void send_button_payload(void) {
    PayloadWriter p(static_cast<PayloadEncoding>(config_get().encoding));
    p.add_button(0x01);
    app_msg_t msg;
    if (p.to_msg(APP_PORT_BUTTON, &msg)) transport_send(&msg);
}

void button_task(void *) {
    app_wdt_add();
    for (;;) {
        app_wdt_feed();
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(APP_WDT_WAIT_MS)) == 0) continue;
        if (!debounced()) continue;  // bounce or release edge

        const uint64_t t0 = hal_sys_uptime_ms();
        bool long_sent = false;
        while (pressed()) {
            app_wdt_feed();
            if (!long_sent && hal_sys_uptime_ms() - t0 >= kLongPressMs) {
                ESP_LOGI(TAG, "long press");
                send_button_payload();
                long_sent = true;
            }
            vTaskDelay(pdMS_TO_TICKS(kPollMs));
        }
        if (!long_sent) {
            ESP_LOGI(TAG, "click: sending count now");
            pax_report_now();
        }
        ulTaskNotifyTake(pdTRUE, 0);  // drop edges collected while held
    }
}

}  // namespace

hal_result_t button_init(void) {
#if BOARD_HAS_BUTTON
    if (xTaskCreatePinnedToCore(button_task, "button", APP_STACK_BUTTON, nullptr, APP_PRIO_INPUT,
                                &s_task, APP_CORE_APP) != pdPASS) {
        return HAL_ERR_NO_MEM;
    }
    const hal_gpio_cfg_t cfg = {BOARD_BUTTON_PIN, HAL_GPIO_DIR_IN,
                                BOARD_BUTTON_ACTIVE_LOW ? HAL_GPIO_PULL_UP : HAL_GPIO_PULL_DOWN,
                                HAL_GPIO_INT_BOTH};
    hal_result_t r = hal_gpio_init(&cfg);
    if (r == HAL_OK) r = hal_gpio_attach_isr(BOARD_BUTTON_PIN, on_edge, nullptr);
    if (r != HAL_OK) return r;
    ESP_LOGI(TAG, "on GPIO%d", BOARD_BUTTON_PIN);
#endif
    return HAL_OK;
}
