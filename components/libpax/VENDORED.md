# libpax (vendored)

- Upstream: https://github.com/dbinfrago/libpax
- Commit: 95758df72ad7981afdff76d1b7f06d4593c77345
- License: Apache-2.0 (see LICENSE)

Local changes:

- `CMakeLists.txt`: explicit source list, LTO removed.
- Leak fix in the stop path (see below). Everything else is unmodified.

Update by copying `lib/libpax/*` from a newer upstream commit, re-applying
the leak fix unless upstream has fixed it, and recording the hash here.

## Local patch: free resources on stop

Upstream `libpax_counter_stop()` stopped but never freed what the matching
start created, so every stop/start cycle (each `pax_restart()`, triggered by
sniffing-related remote commands) leaked:

| Resource | Created in | Leak per restart |
|---|---|---|
| report timer | `libpax_counter_init()` | ~50 B |
| WiFi channel timer | `wifi_sniffer_init()` | ~50 B |
| BLE HCI event task | `start_BLE_scan()` | ~2.2 KB |
| BLE advert queue (60 x ~260 B items) | `start_BLE_scan()` | ~15.6 KB |

The BLE task also kept running, blocked on the abandoned queue.

Fix:

- `libpax_api.cpp` `libpax_counter_stop()`: `xTimerDelete` instead of `xTimerStop`
- `wifiscan.cpp` `wifi_sniffer_stop()`: `xTimerDelete` + clear `WifiChanTimer`
- `blescan.cpp` `stop_BLE_scan()`: after the controller is off, `vTaskDelete`
  the HCI event task, then `vQueueDelete` the advert queue

`xTimerDelete` waits up to 100 ms on the timer command queue, so
`libpax_counter_stop()` must not be called from a timer callback. The firmware
calls it from the rcmd task only.

Candidate for an upstream PR.
