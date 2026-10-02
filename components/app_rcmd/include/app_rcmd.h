// app_rcmd.h - remote command interpreter.
//
// Commands arrive as raw bytes from any channel (console today, LoRaWAN
// downlink later) and are queued with rcmd_submit(). One rcmd task on
// APP_CPU executes them in order, so handlers never run concurrently.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hal_result.h"

#define RCMD_MAX_LEN   32
#define RCMD_QUEUE_LEN 8

// Starts the rcmd task and the console reader task.
hal_result_t rcmd_init(void);

// Non-blocking. Returns false if the queue is full or len is out of range.
bool rcmd_submit(const uint8_t *cmd, size_t len);
