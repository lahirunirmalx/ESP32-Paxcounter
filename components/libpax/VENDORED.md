# libpax (vendored)

- Upstream: https://github.com/dbinfrago/libpax
- Commit: 95758df72ad7981afdff76d1b7f06d4593c77345
- License: Apache-2.0 (see LICENSE)

Local changes are limited to `CMakeLists.txt` (explicit source list, LTO
removed). Source files are unmodified; update by copying `lib/libpax/*` from
a newer upstream commit and recording the hash here.

## Known upstream issue

`libpax_counter_stop()` sets `PaxReportTimer = NULL` without `xTimerDelete()`,
so each `pax_restart()` (triggered by sniffing-related remote commands) leaks
one FreeRTOS timer (~50 bytes). The WiFi channel timer and, with BLE on, the
BLE scan task (2 KB) and its queue leak the same way. The firmware limits this
to one restart per remote-command batch. Candidates for an upstream patch.
