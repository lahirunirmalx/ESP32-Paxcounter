# libpax (vendored)

- Upstream: https://github.com/dbinfrago/libpax
- Commit: 95758df72ad7981afdff76d1b7f06d4593c77345
- License: Apache-2.0 (see LICENSE)

Previously pulled unpinned from GitHub via `lib_deps`. Vendored here so the
memory leak fix below is applied and the version is reproducible.

Update by copying `lib/libpax/*` from a newer upstream commit into `src/`,
re-applying the fix unless upstream has fixed it, and recording the hash here.

## Local patch: free resources on stop

Upstream `libpax_counter_stop()` stopped but never freed what the matching
start created. Every stop/start cycle leaked. In this firmware that is every
remote command that calls `libpax_counter_stop(); init_libpax();` in
`src/rcommand.cpp` (rssi, sendcycle, wifichancycle, wifichanmap, blescantime,
countmode, blescan, wifiscan).

| Resource | Created in | Leak per restart |
|---|---|---|
| report timer | `libpax_counter_init()` | ~50 B |
| WiFi channel timer | `wifi_sniffer_init()` | ~50 B |
| BLE HCI event task | `start_BLE_scan()` | ~2.2 KB |
| BLE advert queue (60 x ~260 B items) | `start_BLE_scan()` | ~15.6 KB |

The BLE task also kept running, blocked on the abandoned queue.

Fix:

- `src/libpax_api.cpp` `libpax_counter_stop()`: `xTimerDelete` instead of `xTimerStop`
- `src/wifiscan.cpp` `wifi_sniffer_stop()`: `xTimerDelete` + clear `WifiChanTimer`
- `src/blescan.cpp` `stop_BLE_scan()`: after the controller is off, `vTaskDelete`
  the HCI event task, then `vQueueDelete` the advert queue

`xTimerDelete` waits up to 100 ms on the timer command queue, so
`libpax_counter_stop()` must not be called from a timer callback. This
firmware calls it from the rcmd task and before deep sleep only.
