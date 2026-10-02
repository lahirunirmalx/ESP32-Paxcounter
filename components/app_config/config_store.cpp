// config_store.cpp - NVS persistence and mutex-guarded access.

#include "app_config.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "hal_kv.h"

namespace {

constexpr char TAG[] = "config";
constexpr char NS[] = "paxcfg";
constexpr char KEY_CFG[] = "cfg";
constexpr char KEY_SCHEMA[] = "schema";
constexpr char KEY_RESTARTS[] = "restarts";

DeviceConfig s_cfg;
SemaphoreHandle_t s_lock = nullptr;
StaticSemaphore_t s_lock_buf;
uint32_t s_restarts = 0;

class Lock {
public:
    Lock() { xSemaphoreTake(s_lock, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(s_lock); }
    Lock(const Lock &) = delete;
    Lock &operator=(const Lock &) = delete;
};

// Reads the stored config into *out. Caller holds the lock.
hal_result_t load_locked(DeviceConfig *out) {
    uint32_t schema = 0;
    hal_result_t r = hal_kv_get_u32(NS, KEY_SCHEMA, &schema);
    if (r != HAL_OK) return r;
    if (schema != CONFIG_SCHEMA_VERSION) {
        ESP_LOGW(TAG, "stored schema %lu != %u, discarding", (unsigned long)schema,
                 CONFIG_SCHEMA_VERSION);
        return HAL_ERR_NOT_FOUND;
    }
    DeviceConfig tmp;
    size_t len = sizeof(tmp);
    r = hal_kv_get_blob(NS, KEY_CFG, &tmp, &len);
    if (r != HAL_OK) return r;
    if (len != sizeof(tmp)) return HAL_ERR_NOT_FOUND;
    *out = tmp;
    return HAL_OK;
}

hal_result_t save_locked(void) {
    hal_result_t r = hal_kv_set_blob(NS, KEY_CFG, &s_cfg, sizeof(s_cfg));
    if (r == HAL_OK) r = hal_kv_set_u32(NS, KEY_SCHEMA, CONFIG_SCHEMA_VERSION);
    return r;
}

}  // namespace

hal_result_t config_init(void) {
    s_lock = xSemaphoreCreateMutexStatic(&s_lock_buf);

    Lock lock;
    hal_result_t r = load_locked(&s_cfg);
    if (r != HAL_OK) {
        ESP_LOGI(TAG, "no valid stored config (%s), using factory defaults", hal_result_name(r));
        config_defaults(&s_cfg);
        r = save_locked();
    } else {
        int fixed = config_sanitize(&s_cfg);
        if (fixed) ESP_LOGW(TAG, "stored config had %d invalid field(s), fixed", fixed);
        if (strncmp(s_cfg.version, PROGVERSION, sizeof(s_cfg.version)) != 0) {
            ESP_LOGI(TAG, "config written by v%.10s, now v%s", s_cfg.version, PROGVERSION);
            memset(s_cfg.version, 0, sizeof(s_cfg.version));
            strncpy(s_cfg.version, PROGVERSION, sizeof(s_cfg.version) - 1);
            r = save_locked();
        }
    }

    if (hal_kv_get_u32(NS, KEY_RESTARTS, &s_restarts) != HAL_OK) s_restarts = 0;
    s_restarts++;
    hal_kv_set_u32(NS, KEY_RESTARTS, s_restarts);
    ESP_LOGI(TAG, "loaded, boot #%lu", (unsigned long)s_restarts);
    return r;
}

DeviceConfig config_get(void) {
    Lock lock;
    return s_cfg;
}

void config_update(config_mutator_t fn, void *ctx) {
    if (fn == nullptr) return;
    Lock lock;
    fn(&s_cfg, ctx);
    config_sanitize(&s_cfg);
}

hal_result_t config_save(void) {
    Lock lock;
    hal_result_t r = save_locked();
    ESP_LOGI(TAG, "save: %s", hal_result_name(r));
    return r;
}

hal_result_t config_reload(void) {
    Lock lock;
    DeviceConfig tmp;
    hal_result_t r = load_locked(&tmp);
    if (r == HAL_OK) {
        config_sanitize(&tmp);
        s_cfg = tmp;
    }
    return r;
}

hal_result_t config_factory_reset(void) {
    Lock lock;
    hal_kv_erase_ns(NS);
    config_defaults(&s_cfg);
    hal_result_t r = save_locked();
    hal_kv_set_u32(NS, KEY_RESTARTS, s_restarts);
    ESP_LOGW(TAG, "factory reset: %s", hal_result_name(r));
    return r;
}

uint32_t config_restart_count(void) {
    return s_restarts;
}
