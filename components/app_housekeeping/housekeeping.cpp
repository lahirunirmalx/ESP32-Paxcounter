// housekeeping.cpp - periodic health check task.

#include "app_housekeeping.h"

#include <stdlib.h>

#include <memory>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_tasks.h"
#include "app_transport.h"
#include "app_wdt.h"
#include "hal_sys.h"

namespace {

constexpr char TAG[] = "housekeeping";

struct FreeDeleter {
    void operator()(void *p) const { free(p); }
};

void log_tasks(void) {
    const UBaseType_t n = uxTaskGetNumberOfTasks() + 2;  // slack for tasks created meanwhile
    std::unique_ptr<TaskStatus_t, FreeDeleter> tasks(
        static_cast<TaskStatus_t *>(malloc(n * sizeof(TaskStatus_t))));
    if (!tasks) return;
    const UBaseType_t got = uxTaskGetSystemState(tasks.get(), n, nullptr);
    for (UBaseType_t i = 0; i < got; i++) {
        const TaskStatus_t &t = tasks.get()[i];
        const uint32_t free_bytes = t.usStackHighWaterMark;  // bytes on ESP-IDF
        const int core = (t.xCoreID == tskNO_AFFINITY) ? -1 : static_cast<int>(t.xCoreID);
        if (free_bytes < HOUSEKEEPING_LOW_STACK) {
            ESP_LOGW(TAG, "task %-12s core %2d prio %2u stack free %4lu  LOW", t.pcTaskName, core,
                     static_cast<unsigned>(t.uxCurrentPriority), static_cast<unsigned long>(free_bytes));
        } else {
            ESP_LOGD(TAG, "task %-12s core %2d prio %2u stack free %4lu", t.pcTaskName, core,
                     static_cast<unsigned>(t.uxCurrentPriority), static_cast<unsigned long>(free_bytes));
        }
    }
}

void housekeeping_task(void *) {
    app_wdt_add();
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        // Feed in short slices so the period can exceed the WDT timeout.
        for (int s = 0; s < HOUSEKEEPING_PERIOD_S; s++) {
            app_wdt_feed();
            vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
        }
        const uint32_t heap = hal_sys_free_heap();
        ESP_LOGI(TAG, "uptime %llus heap free %lu min %lu, tx queued %lu dropped %lu",
                 static_cast<unsigned long long>(hal_sys_uptime_ms() / 1000),
                 static_cast<unsigned long>(heap), static_cast<unsigned long>(hal_sys_min_free_heap()),
                 static_cast<unsigned long>(transport_queued()),
                 static_cast<unsigned long>(transport_dropped()));
        if (heap < HOUSEKEEPING_LOW_HEAP) ESP_LOGW(TAG, "low heap: %lu bytes", static_cast<unsigned long>(heap));
        log_tasks();
    }
}

}  // namespace

hal_result_t housekeeping_init(void) {
    return xTaskCreatePinnedToCore(housekeeping_task, "housekeep", APP_STACK_HOUSEKEEP, nullptr,
                                   APP_PRIO_BACKGROUND, nullptr, APP_CORE_APP) == pdPASS
               ? HAL_OK
               : HAL_ERR_NO_MEM;
}
