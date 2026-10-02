// app_transport.h - outbound payload path.
//
// Producers (pax report, remote command replies, button) call
// transport_send(); it copies the message into a bounded queue and returns
// immediately. One transport task on PRO_CPU drains the queue and hands each
// message to every registered sink (console now; LoRaWAN / MQTT / SD later).

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_msg.h"
#include "hal_result.h"

#define TRANSPORT_QUEUE_LEN 10
#define TRANSPORT_MAX_SINKS 4

typedef struct {
    const char *name;
    hal_result_t (*send)(const app_msg_t *msg);
} transport_sink_t;

hal_result_t transport_init(void);

// sink must outlive the transport (use a static object).
hal_result_t transport_add_sink(const transport_sink_t *sink);

// Non-blocking. Returns false (and counts a drop) if the queue is full.
bool transport_send(const app_msg_t *msg);

// Discards everything still queued.
void transport_flush(void);

uint32_t transport_queued(void);
uint32_t transport_dropped(void);

// Built-in sink: logs each payload as one hex line on the console.
extern const transport_sink_t transport_sink_console;
