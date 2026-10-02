// app_msg.h - message passed from producers to the transport task.

#pragma once

#include <stdint.h>

#define APP_PAYLOAD_MAX 51   // max payload per transmit (LoRaWAN DR0 safe size)

typedef struct {
    uint8_t size;                    // valid bytes in data[]
    uint8_t port;                    // APP_PORT_*
    uint8_t data[APP_PAYLOAD_MAX];
} app_msg_t;
