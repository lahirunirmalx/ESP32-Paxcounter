// app_main.cpp - boot sequence and task supervisor.
//
// Start order follows the dependencies:
//   storage -> config -> LED -> transport (+ sinks) -> pax counting
//   -> remote commands -> button -> housekeeping
// Each feature owns its task(s); see components/app_common/include/app_tasks.h
// for the core / priority map. app_main returns once everything is running.

#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_button.h"
#include "app_config.h"
#include "app_housekeeping.h"
#include "app_led.h"
#include "app_pax.h"
#include "app_rcmd.h"
#include "app_transport.h"
#include "board.h"
#include "hal_kv.h"
#include "hal_sys.h"

static constexpr char TAG[] = "main";

// Boot-time invariant: if a core service cannot start there is nothing
// useful to run, so log, show the error on the LED and restart.
static void require(hal_result_t r, const char *what) {
    if (r == HAL_OK) return;
    ESP_LOGE(TAG, "%s failed: %s - restarting in 5 s", what, hal_result_name(r));
    led_set_mode(LED_MODE_ERROR);
    vTaskDelay(pdMS_TO_TICKS(5000));
    hal_sys_restart();
}

static void warn_if(hal_result_t r, const char *what) {
    if (r != HAL_OK) ESP_LOGW(TAG, "%s failed: %s (continuing)", what, hal_result_name(r));
}

extern "C" void app_main(void) {
    uint8_t mac[6] = {};
    hal_sys_mac(mac);
    ESP_LOGI(TAG, "Paxcounter v%s on %s, id %02x%02x%02x%02x%02x%02x, reset reason %d", PROGVERSION,
             BOARD_NAME, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             static_cast<int>(hal_sys_reset_reason()));

    require(hal_kv_init(), "storage");
    warn_if(config_init(), "config save");
    warn_if(led_init(), "LED");

    // The WiFi driver posts events to the default loop.
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    require(transport_init(), "transport");
    warn_if(transport_add_sink(&transport_sink_console), "console sink");
    warn_if(transport_add_sink(&led_activity_sink), "LED sink");

    require(pax_start(), "pax counter");
    require(rcmd_init(), "remote commands");
    warn_if(button_init(), "button");
    require(housekeeping_init(), "housekeeping");

    led_set_mode(LED_MODE_RUN);
    ESP_LOGI(TAG, "running. Send hex commands on the console, e.g. '80' = get config");
}
