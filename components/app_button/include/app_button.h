// app_button.h - on-board button.
//
//   short click (< 1 s)  send a counter payload now
//   long press  (>= 1 s) send a button payload (0x01) on port 5, as the
//                        legacy firmware did

#pragma once

#include "hal_result.h"

hal_result_t button_init(void);
