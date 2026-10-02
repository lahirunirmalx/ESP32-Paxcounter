// port_kv.cpp - NVS implementation of hal_kv.h.

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "hal_kv.h"
#include "port_err.h"

namespace {

constexpr char TAG[] = "port_kv";

// RAII wrapper so every exit path closes the NVS handle.
class NvsHandle {
public:
    NvsHandle(const char *ns, nvs_open_mode_t mode) { err_ = nvs_open(ns, mode, &h_); }
    ~NvsHandle() {
        if (err_ == ESP_OK) nvs_close(h_);
    }
    NvsHandle(const NvsHandle &) = delete;
    NvsHandle &operator=(const NvsHandle &) = delete;

    esp_err_t err() const { return err_; }
    nvs_handle_t get() const { return h_; }

private:
    nvs_handle_t h_ = 0;
    esp_err_t err_ = ESP_FAIL;
};

}  // namespace

extern "C" hal_result_t hal_kv_init(void) {
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS layout unreadable (%s), erasing", esp_err_to_name(e));
        e = nvs_flash_erase();
        if (e == ESP_OK) e = nvs_flash_init();
    }
    return port_map_err(e);
}

extern "C" hal_result_t hal_kv_get_blob(const char *ns, const char *key, void *buf, size_t *len) {
    if (ns == nullptr || key == nullptr || buf == nullptr || len == nullptr) return HAL_ERR_INVALID_ARG;
    NvsHandle h(ns, NVS_READONLY);
    if (h.err() != ESP_OK) return port_map_err(h.err());
    return port_map_err(nvs_get_blob(h.get(), key, buf, len));
}

extern "C" hal_result_t hal_kv_set_blob(const char *ns, const char *key, const void *buf, size_t len) {
    if (ns == nullptr || key == nullptr || buf == nullptr) return HAL_ERR_INVALID_ARG;
    NvsHandle h(ns, NVS_READWRITE);
    if (h.err() != ESP_OK) return port_map_err(h.err());
    esp_err_t e = nvs_set_blob(h.get(), key, buf, len);
    if (e == ESP_OK) e = nvs_commit(h.get());
    return port_map_err(e);
}

extern "C" hal_result_t hal_kv_get_u32(const char *ns, const char *key, uint32_t *value) {
    if (ns == nullptr || key == nullptr || value == nullptr) return HAL_ERR_INVALID_ARG;
    NvsHandle h(ns, NVS_READONLY);
    if (h.err() != ESP_OK) return port_map_err(h.err());
    return port_map_err(nvs_get_u32(h.get(), key, value));
}

extern "C" hal_result_t hal_kv_set_u32(const char *ns, const char *key, uint32_t value) {
    if (ns == nullptr || key == nullptr) return HAL_ERR_INVALID_ARG;
    NvsHandle h(ns, NVS_READWRITE);
    if (h.err() != ESP_OK) return port_map_err(h.err());
    esp_err_t e = nvs_set_u32(h.get(), key, value);
    if (e == ESP_OK) e = nvs_commit(h.get());
    return port_map_err(e);
}

extern "C" hal_result_t hal_kv_erase_ns(const char *ns) {
    if (ns == nullptr) return HAL_ERR_INVALID_ARG;
    NvsHandle h(ns, NVS_READWRITE);
    if (h.err() != ESP_OK) return port_map_err(h.err());
    esp_err_t e = nvs_erase_all(h.get());
    if (e == ESP_OK) e = nvs_commit(h.get());
    return port_map_err(e);
}
