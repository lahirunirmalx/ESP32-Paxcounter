// hal_kv.h - non-volatile key/value storage.
//
// Backed by NVS on ESP-IDF. Routed through the HAL so configuration logic
// can be unit-tested on the host with an in-memory port.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hal_result.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mounts the store. Erases and re-mounts it if the on-flash layout is
// unreadable (no free pages / new format version).
hal_result_t hal_kv_init(void);

// *len is buffer size on input, stored size on output.
// HAL_ERR_NOT_FOUND if the key does not exist.
hal_result_t hal_kv_get_blob(const char *ns, const char *key, void *buf, size_t *len);
hal_result_t hal_kv_set_blob(const char *ns, const char *key, const void *buf, size_t len);

hal_result_t hal_kv_get_u32(const char *ns, const char *key, uint32_t *value);
hal_result_t hal_kv_set_u32(const char *ns, const char *key, uint32_t value);

// Removes every key in the namespace.
hal_result_t hal_kv_erase_ns(const char *ns);

#ifdef __cplusplus
}
#endif
