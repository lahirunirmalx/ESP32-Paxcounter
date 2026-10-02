// app_wdt.h - task watchdog helper.
//
// A task calls app_wdt_add() once at start-up and app_wdt_feed() at least
// once per CONFIG_ESP_TASK_WDT_TIMEOUT_S. Tasks that block on a queue use a
// bounded wait (APP_WDT_WAIT_MS) and feed on every wake-up.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_WDT_WAIT_MS 2000u

void app_wdt_add(void);
void app_wdt_feed(void);

#ifdef __cplusplus
}
#endif
