// app_tasks.h - every task's core, priority and stack size in one place.
//
// ESP32 dual-core split:
//   PRO_CPU (core 0): WiFi + BT controller + libpax timers (anchored there by
//                     ESP-IDF), plus I/O-bound work: payload transport and
//                     console input.
//   APP_CPU (core 1): application logic, kept away from the radio stacks:
//                     count reporting, remote commands, button, LED,
//                     housekeeping.
// Single-core SoCs pin everything to core 0.

#pragma once

#include "freertos/FreeRTOS.h"
#include "soc/soc_caps.h"

#if SOC_CPU_CORES_NUM >= 2
#define APP_CORE_NET 0
#define APP_CORE_APP 1
#else
#define APP_CORE_NET 0
#define APP_CORE_APP 0
#endif

// Priority bands. Anything above 7 competes with the IDF event loop and the
// WiFi stack.
#define APP_PRIO_BACKGROUND  (tskIDLE_PRIORITY + 1)  // LED, housekeeping
#define APP_PRIO_IO          (tskIDLE_PRIORITY + 2)  // transport, console
#define APP_PRIO_CONTROL     (tskIDLE_PRIORITY + 3)  // remote commands
#define APP_PRIO_DATA        (tskIDLE_PRIORITY + 4)  // pax report
#define APP_PRIO_INPUT       (tskIDLE_PRIORITY + 5)  // button

// Stack sizes in bytes.
#define APP_STACK_PAX       4096
#define APP_STACK_TRANSPORT 4096
#define APP_STACK_CONSOLE   3072
#define APP_STACK_RCMD      4096
#define APP_STACK_BUTTON    3072
#define APP_STACK_LED       2048
#define APP_STACK_HOUSEKEEP 3072
