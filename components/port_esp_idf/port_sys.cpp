// port_sys.cpp - ESP-IDF implementation of hal_sys.h.

#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "hal_sys.h"
#include "port_err.h"

extern "C" hal_result_t hal_sys_mac(uint8_t mac[6]) {
    if (mac == nullptr) return HAL_ERR_INVALID_ARG;
    return port_map_err(esp_read_mac(mac, ESP_MAC_WIFI_STA));
}

extern "C" uint64_t hal_sys_uptime_ms(void) {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

extern "C" uint32_t hal_sys_free_heap(void) {
    return static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
}

extern "C" uint32_t hal_sys_min_free_heap(void) {
    return static_cast<uint32_t>(heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
}

extern "C" uint32_t hal_sys_random(void) { return esp_random(); }

extern "C" hal_reset_reason_t hal_sys_reset_reason(void) {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return HAL_RESET_POWERON;
        case ESP_RST_EXT:       return HAL_RESET_EXTERNAL;
        case ESP_RST_SW:        return HAL_RESET_SOFTWARE;
        case ESP_RST_PANIC:     return HAL_RESET_PANIC;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:       return HAL_RESET_WATCHDOG;
        case ESP_RST_DEEPSLEEP: return HAL_RESET_DEEPSLEEP;
        case ESP_RST_BROWNOUT:  return HAL_RESET_BROWNOUT;
        default:                return HAL_RESET_UNKNOWN;
    }
}

extern "C" hal_result_t hal_sys_cpu_temp(float *celsius) {
    if (celsius == nullptr) return HAL_ERR_INVALID_ARG;
    // The classic ESP32 has no supported on-die temperature sensor driver.
    *celsius = 0.0f;
    return HAL_ERR_UNSUPPORTED;
}

extern "C" void hal_sys_restart(void) {
    esp_restart();
}
