// hal_uart.h - vendor-neutral UART interface.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hal_result.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_UART_PIN_UNCHANGED (-1)

typedef struct {
    uint8_t  port;          // UART controller number
    uint32_t baud;
    int      tx_pin;        // HAL_UART_PIN_UNCHANGED keeps the default pin
    int      rx_pin;        // HAL_UART_PIN_UNCHANGED keeps the default pin
    size_t   rx_buf_size;   // driver RX ring buffer, bytes (> 128)
} hal_uart_cfg_t;

hal_result_t hal_uart_init(const hal_uart_cfg_t *cfg);

// Blocks up to timeout_ms. Returns HAL_OK with *read_out == 0 on timeout.
hal_result_t hal_uart_read(uint8_t port, uint8_t *buf, size_t len,
                           uint32_t timeout_ms, size_t *read_out);

hal_result_t hal_uart_write(uint8_t port, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
