// app_config.h - thread-safe access to the device configuration.
//
// One owner (this component) holds the live DeviceConfig behind a mutex.
// Readers get a copy; writers pass a mutation function that runs under the
// lock. Nothing else in the firmware keeps a global config.

#pragma once

#include "config_types.h"
#include "hal_result.h"

// Loads the config from non-volatile storage, falling back to factory
// defaults if nothing valid is stored. Call once, after hal_kv_init().
hal_result_t config_init(void);

// Snapshot of the current configuration.
DeviceConfig config_get(void);

// Runs fn(cfg, ctx) under the config lock, then sanitizes the result.
// Changes are in RAM only until config_save() is called.
typedef void (*config_mutator_t)(DeviceConfig *cfg, void *ctx);
void config_update(config_mutator_t fn, void *ctx);

hal_result_t config_save(void);
hal_result_t config_reload(void);

// Erases the stored config and restores factory defaults (RAM + flash).
hal_result_t config_factory_reset(void);

// Persistent boot counter, incremented once per boot by config_init().
uint32_t config_restart_count(void);
