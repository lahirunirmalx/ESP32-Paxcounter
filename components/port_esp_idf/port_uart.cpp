// port_uart.cpp - ESP-IDF implementation of hal_uart.h.

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

#include "hal_uart.h"
#include "port_err.h"

extern "C" hal_result_t hal_uart_init(const hal_uart_cfg_t *cfg) {
    if (cfg == nullptr || cfg->port >= UART_NUM_MAX || cfg->rx_buf_size <= 128) {
        return HAL_ERR_INVALID_ARG;
    }
    const auto port = static_cast<uart_port_t>(cfg->port);

    uart_config_t uc = {};
    uc.baud_rate  = static_cast<int>(cfg->baud);
    uc.data_bits  = UART_DATA_8_BITS;
    uc.parity     = UART_PARITY_DISABLE;
    uc.stop_bits  = UART_STOP_BITS_1;
    uc.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    uc.source_clk = UART_SCLK_DEFAULT;

    hal_result_t r = port_map_err(
        uart_driver_install(port, static_cast<int>(cfg->rx_buf_size), 0, 0, nullptr, 0));
    if (r != HAL_OK) return r;
    r = port_map_err(uart_param_config(port, &uc));
    if (r != HAL_OK) return r;
    return port_map_err(uart_set_pin(port,
                                     cfg->tx_pin == HAL_UART_PIN_UNCHANGED ? UART_PIN_NO_CHANGE : cfg->tx_pin,
                                     cfg->rx_pin == HAL_UART_PIN_UNCHANGED ? UART_PIN_NO_CHANGE : cfg->rx_pin,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

extern "C" hal_result_t hal_uart_read(uint8_t port, uint8_t *buf, size_t len,
                                      uint32_t timeout_ms, size_t *read_out) {
    if (buf == nullptr || read_out == nullptr || port >= UART_NUM_MAX) return HAL_ERR_INVALID_ARG;
    int n = uart_read_bytes(static_cast<uart_port_t>(port), buf, static_cast<uint32_t>(len),
                            pdMS_TO_TICKS(timeout_ms));
    if (n < 0) {
        *read_out = 0;
        return HAL_ERR_HW_FAULT;
    }
    *read_out = static_cast<size_t>(n);
    return HAL_OK;
}

extern "C" hal_result_t hal_uart_write(uint8_t port, const uint8_t *buf, size_t len) {
    if (buf == nullptr || port >= UART_NUM_MAX) return HAL_ERR_INVALID_ARG;
    int n = uart_write_bytes(static_cast<uart_port_t>(port), buf, len);
    return (n == static_cast<int>(len)) ? HAL_OK : HAL_ERR_HW_FAULT;
}
