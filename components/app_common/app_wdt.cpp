#include "app_wdt.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

static constexpr char TAG[] = "wdt";

extern "C" void app_wdt_add(void) {
    esp_err_t e = esp_task_wdt_add(nullptr);
    if (e != ESP_OK) ESP_LOGW(TAG, "esp_task_wdt_add: %s", esp_err_to_name(e));
}

extern "C" void app_wdt_feed(void) {
    esp_task_wdt_reset();
}
