// rcmd_task.cpp - command queue + executor task, console line reader task.

#include "app_rcmd.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "app_pax.h"
#include "app_tasks.h"
#include "app_wdt.h"
#include "board.h"
#include "hal_uart.h"
#include "rcmd_parser.h"

extern const RcmdEntry g_rcmd_table[];
extern const size_t g_rcmd_table_len;
bool rcmd_take_pax_restart(void);

namespace {

constexpr char TAG[] = "rcmd";
constexpr size_t kLineMax = RCMD_MAX_LEN * 3 + 8;  // "xx " per byte + slack

struct RcmdBuf {
    uint8_t len;
    uint8_t data[RCMD_MAX_LEN];
};

QueueHandle_t s_queue = nullptr;

void rcmd_task(void *) {
    app_wdt_add();
    RcmdBuf buf;
    for (;;) {
        app_wdt_feed();
        if (xQueueReceive(s_queue, &buf, pdMS_TO_TICKS(APP_WDT_WAIT_MS)) != pdTRUE) continue;
        const RcmdResult r = rcmd_execute(buf.data, buf.len, g_rcmd_table, g_rcmd_table_len, nullptr);
        switch (r.status) {
            case RCMD_UNKNOWN_OPCODE:
                ESP_LOGW(TAG, "unknown opcode 0x%02x (%d command(s) run before it)", r.bad_opcode,
                         r.executed);
                break;
            case RCMD_MISSING_ARGS:
                ESP_LOGW(TAG, "opcode 0x%02x is missing arguments, skipped", r.bad_opcode);
                break;
            case RCMD_OK:
            case RCMD_EMPTY:
            default:
                break;
        }
        if (rcmd_take_pax_restart()) {
            app_wdt_feed();
            ESP_LOGI(TAG, "applying sniffer settings: %s", hal_result_name(pax_restart()));
        }
    }
}

// Reads hex command lines from the USB-serial console, e.g.
//   0a 0f      -> set send cycle to 30 s
//   80         -> get config (reply on port 3)
// Lines starting with '#' are ignored.
void console_task(void *) {
    app_wdt_add();
    char line[kLineMax];
    size_t n = 0;
    bool overflow = false;
    for (;;) {
        app_wdt_feed();
        uint8_t ch;
        size_t got = 0;
        if (hal_uart_read(BOARD_CONSOLE_UART, &ch, 1, 1000, &got) != HAL_OK || got == 0) continue;

        if (ch != '\n' && ch != '\r') {
            if (n < sizeof(line) - 1) {
                line[n++] = static_cast<char>(ch);
            } else {
                overflow = true;
            }
            continue;
        }
        if (n == 0) continue;
        line[n] = '\0';
        if (overflow) {
            ESP_LOGW(TAG, "console line too long, ignored");
        } else if (line[0] != '#') {
            uint8_t cmd[RCMD_MAX_LEN];
            const size_t len = rcmd_hex_decode(line, cmd, sizeof(cmd));
            if (len == 0) {
                ESP_LOGW(TAG, "not a hex command: '%s'", line);
            } else {
                rcmd_submit(cmd, len);
            }
        }
        n = 0;
        overflow = false;
    }
}

}  // namespace

hal_result_t rcmd_init(void) {
    s_queue = xQueueCreate(RCMD_QUEUE_LEN, sizeof(RcmdBuf));
    if (s_queue == nullptr) return HAL_ERR_NO_MEM;

    if (xTaskCreatePinnedToCore(rcmd_task, "rcmd", APP_STACK_RCMD, nullptr, APP_PRIO_CONTROL,
                                nullptr, APP_CORE_APP) != pdPASS) {
        return HAL_ERR_NO_MEM;
    }

    const hal_uart_cfg_t uart = {BOARD_CONSOLE_UART, BOARD_CONSOLE_BAUD, HAL_UART_PIN_UNCHANGED,
                                 HAL_UART_PIN_UNCHANGED, 256};
    hal_result_t r = hal_uart_init(&uart);
    if (r != HAL_OK) {
        ESP_LOGW(TAG, "console input unavailable: %s", hal_result_name(r));
        return HAL_OK;  // commands can still arrive from other channels
    }
    if (xTaskCreatePinnedToCore(console_task, "console", APP_STACK_CONSOLE, nullptr, APP_PRIO_IO,
                                nullptr, APP_CORE_NET) != pdPASS) {
        return HAL_ERR_NO_MEM;
    }
    return HAL_OK;
}

bool rcmd_submit(const uint8_t *cmd, size_t len) {
    if (s_queue == nullptr || cmd == nullptr || len == 0 || len > RCMD_MAX_LEN) return false;
    RcmdBuf buf;
    buf.len = static_cast<uint8_t>(len);
    memcpy(buf.data, cmd, len);
    if (xQueueSendToBack(s_queue, &buf, 0) != pdTRUE) {
        ESP_LOGW(TAG, "command queue full");
        return false;
    }
    return true;
}
