// app_led.h - status LED.
//
//   LED_MODE_BOOT  fast blink while starting up
//   LED_MODE_RUN   short heartbeat pulse every 2 s
//   LED_MODE_ERROR solid on
// led_flash() adds a single short flash on top of the current mode.

#pragma once

#include "app_transport.h"
#include "hal_result.h"

typedef enum {
    LED_MODE_BOOT = 0,
    LED_MODE_RUN,
    LED_MODE_ERROR,
} led_mode_t;

hal_result_t led_init(void);
void led_set_mode(led_mode_t mode);
void led_flash(void);

// Transport sink that flashes the LED once per sent payload.
extern const transport_sink_t led_activity_sink;
