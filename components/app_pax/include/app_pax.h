// app_pax.h - pax counting.
//
// libpax sniffs WiFi probe requests and BLE adverts. Every send cycle its
// timer callback copies the count into a one-slot mailbox; the pax report
// task (APP_CPU) wakes up, encodes the counter payload and queues it for
// transport. The timer callback never blocks.

#pragma once

#include <stdint.h>

#include "hal_result.h"

typedef struct {
    uint32_t pax;
    uint32_t wifi;
    uint32_t ble;
    uint64_t at_ms;   // uptime when the count was taken
} pax_count_t;

// Starts sniffing with the current config and spawns the report task.
hal_result_t pax_start(void);

// Stops and restarts libpax with the current config. Call after changing
// any sniffing parameter (rssi, channels, scan on/off, send cycle, mode).
hal_result_t pax_restart(void);

// Sends a counter payload now with the live (unreset) count.
void pax_report_now(void);

// Most recent count (from the last report or report_now).
pax_count_t pax_last(void);
