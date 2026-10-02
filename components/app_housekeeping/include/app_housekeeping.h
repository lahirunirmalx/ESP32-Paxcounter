// app_housekeeping.h - periodic health check (every HOUSEKEEPING_PERIOD_S).
//
// Logs free heap, the minimum ever free heap, every task's unused stack and
// the transport drop counter. Warns when heap drops below
// HOUSEKEEPING_LOW_HEAP or a task has less than HOUSEKEEPING_LOW_STACK bytes
// of stack left.

#pragma once

#include "hal_result.h"

#define HOUSEKEEPING_PERIOD_S   30
#define HOUSEKEEPING_LOW_HEAP   (16 * 1024)
#define HOUSEKEEPING_LOW_STACK  512

hal_result_t housekeeping_init(void);
