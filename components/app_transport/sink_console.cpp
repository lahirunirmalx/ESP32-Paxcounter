// sink_console.cpp - prints each payload as one parseable line:
//   I (12345) payload: port=1 len=4 hex=0a000300

#include "app_transport.h"

#include "esp_log.h"

static constexpr char TAG[] = "payload";

static hal_result_t console_send(const app_msg_t *msg) {
    static constexpr char kHex[] = "0123456789abcdef";
    char hex[APP_PAYLOAD_MAX * 2 + 1];
    size_t n = 0;
    for (uint8_t i = 0; i < msg->size && i < APP_PAYLOAD_MAX; i++) {
        hex[n++] = kHex[msg->data[i] >> 4];
        hex[n++] = kHex[msg->data[i] & 0x0F];
    }
    hex[n] = '\0';
    ESP_LOGI(TAG, "port=%u len=%u hex=%s", msg->port, msg->size, hex);
    return HAL_OK;
}

const transport_sink_t transport_sink_console = {"console", console_send};
