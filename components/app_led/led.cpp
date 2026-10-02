// led.cpp - LED pattern task.

#include "app_led.h"

#include <atomic>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_tasks.h"
#include "board.h"
#include "hal_gpio.h"

namespace {

constexpr char TAG[] = "led";
constexpr uint32_t kNotifyFlash = 1u << 0;
constexpr uint32_t kNotifyMode = 1u << 1;

TaskHandle_t s_task = nullptr;
std::atomic<int> s_mode{LED_MODE_BOOT};

void set(bool on) {
    hal_gpio_set(BOARD_LED_PIN, BOARD_LED_ACTIVE_HIGH ? on : !on);
}

// Waits for the given time or until notified. Returns the notify bits.
uint32_t wait(uint32_t ms) {
    uint32_t bits = 0;
    xTaskNotifyWait(0, UINT32_MAX, &bits, pdMS_TO_TICKS(ms));
    return bits;
}

void led_task(void *) {
    for (;;) {
        uint32_t bits = 0;
        switch (static_cast<led_mode_t>(s_mode.load())) {
            case LED_MODE_BOOT:
                set(true);
                bits |= wait(100);
                set(false);
                bits |= wait(100);
                break;
            case LED_MODE_ERROR:
                set(true);
                bits |= wait(1000);
                break;
            case LED_MODE_RUN:
            default:
                set(true);
                bits |= wait(30);
                set(false);
                bits |= wait(1970);
                break;
        }
        if (bits & kNotifyFlash) {
            set(true);
            vTaskDelay(pdMS_TO_TICKS(80));
            set(false);
            vTaskDelay(pdMS_TO_TICKS(80));
        }
    }
}

hal_result_t activity_send(const app_msg_t *) {
    led_flash();
    return HAL_OK;
}

}  // namespace

const transport_sink_t led_activity_sink = {"led", activity_send};

hal_result_t led_init(void) {
#if BOARD_HAS_LED
    const hal_gpio_cfg_t cfg = {BOARD_LED_PIN, HAL_GPIO_DIR_OUT, HAL_GPIO_PULL_NONE, HAL_GPIO_INT_NONE};
    hal_result_t r = hal_gpio_init(&cfg);
    if (r != HAL_OK) return r;
    set(false);
    if (xTaskCreatePinnedToCore(led_task, "led", APP_STACK_LED, nullptr, APP_PRIO_BACKGROUND,
                                &s_task, APP_CORE_APP) != pdPASS) {
        return HAL_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "on GPIO%d", BOARD_LED_PIN);
#endif
    return HAL_OK;
}

void led_set_mode(led_mode_t mode) {
    s_mode.store(mode);
    if (s_task != nullptr) xTaskNotify(s_task, kNotifyMode, eSetBits);
}

void led_flash(void) {
    if (s_task != nullptr) xTaskNotify(s_task, kNotifyFlash, eSetBits);
}
