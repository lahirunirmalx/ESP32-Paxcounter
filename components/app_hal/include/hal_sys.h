// hal_sys.h - system services: identity, uptime, memory, reset.
//
// Plain getters that cannot fail return their value directly; everything
// that can fail returns hal_result_t.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hal_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_RESET_UNKNOWN = 0,
    HAL_RESET_POWERON,
    HAL_RESET_EXTERNAL,
    HAL_RESET_SOFTWARE,
    HAL_RESET_PANIC,
    HAL_RESET_WATCHDOG,
    HAL_RESET_DEEPSLEEP,
    HAL_RESET_BROWNOUT
} hal_reset_reason_t;

hal_result_t hal_sys_mac(uint8_t mac[6]);   // factory WiFi STA MAC

uint64_t hal_sys_uptime_ms(void);
uint32_t hal_sys_free_heap(void);
uint32_t hal_sys_min_free_heap(void);
uint32_t hal_sys_random(void);
hal_reset_reason_t hal_sys_reset_reason(void);

// HAL_ERR_UNSUPPORTED on SoCs without an on-die temperature sensor.
hal_result_t hal_sys_cpu_temp(float *celsius);

void hal_sys_restart(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
