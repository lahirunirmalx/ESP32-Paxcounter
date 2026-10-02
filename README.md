# ESP32-Paxcounter
![logo](docs/assets/paxcounter_logo_white.png)

**Wifi & Bluetooth driven, LoRaWAN enabled, Paxcounter and multi-sensor appliance, built on cheap ESP32 LoRa IoT boards**

[Tutorial (in german language): heise.de](https://www.heise.de/select/make/2019/1/1551099236518668)

[![CodeFactor](https://www.codefactor.io/repository/github/cyberman54/esp32-paxcounter/badge)](https://www.codefactor.io/repository/github/cyberman54/esp32-paxcounter)
[![PlatformIO CI](https://github.com/cyberman54/ESP32-Paxcounter/actions/workflows/build.yml/badge.svg?event=push)](https://github.com/cyberman54/ESP32-Paxcounter/actions/workflows/build.yml)

---

**Ready-to-go Hardware**: <a href="https://de.aliexpress.com/item/32915894264.html" target="_blank">LILYGO® Paxcounter LoRa</a>

**Documentation**: <a href="https://paxcounter.org" target="_blank">https://paxcounter.org</a>

**Source Code**: <a href="https://github.com/cyberman54/ESP32-Paxcounter" target="_blank">https://github.com/cyberman54/ESP32-Paxcounter</a>

---

<img src="docs/img/Paxcounter-title.jpg">
<img src="docs/img/Paxcounter-ttgo.jpg">
<img src="docs/img/Paxcounter-lolin.gif">
<img src="docs/img/Paxcounter-Screen.png">
<img src="docs/img/TTGO-case.jpg">
<img src="docs/img/TTGO-curves.jpg">
<img src="docs/img/Paxcounter-LEDmatrix.jpg">
<img src="docs/img/Paxcounter-Clock2.png">
<img src="docs/img/Paxcounter-ttgo-twristband.jpg">

# Use case

Paxcounter is an [ESP32](https://www.espressif.com/en/products/socs/esp32) MCU based device for metering passenger flows and multi-sensor data in realtime. It counts how many mobile devices are around. This gives an estimation how many people are around. Paxcounter detects Wifi and Bluetooth signals in the air, focusing on mobile devices by evaluating their MAC adresses. In parallel, it reads and stores data from multiple connected environment sensors.

Intention of this project is to do this without intrusion in privacy: You don't need to track people owned devices, if you just want to count them. Therefore, Paxcounter does not persistenly store MAC adresses and does no kind of fingerprinting the scanned devices.

Data can either be stored on a local SD-card, transferred to cloud using LoRaWAN network (e.g. TheThingsNetwork or Helium) or MQTT over TCP/IP, or transmitted to a local host using serial (SPI) interface.

You can build this project battery powered using ESP32 deep sleep mode and reach long uptimes with a single 18650 Li-Ion cell.

# Native ESP-IDF rewrite (branch `feature/espidf-hal-rewrite`)

This branch replaces the Arduino code base with a native ESP-IDF 5.3 firmware
built around a hardware abstraction layer (HAL) and one FreeRTOS task per
feature. The Arduino version remains on `master`.

## Build, flash, test

```
pio run                      # build esp32_dev (generic ESP32-WROOM-32 devkit)
pio run -t upload -t monitor # flash and open the serial console
pio test -e native           # host unit tests (payload, commands, config)
pio run -t erase             # wipe flash incl. stored config
```

## Layout

| Path | Role |
|---|---|
| `main/app_main.cpp` | boot sequence, starts every feature |
| `components/app_hal/` | vendor-neutral interfaces: `hal_gpio`, `hal_uart`, `hal_kv`, `hal_sys` |
| `components/port_esp_idf/` | ESP-IDF implementation of the HAL, the only code that includes `driver/*.h` |
| `components/app_board/` | pin map (`board.h`) |
| `components/app_config/` | runtime config: defaults/validation (pure) + NVS store behind a mutex |
| `components/app_payload/` | plain / packed payload encoder, byte-compatible with the legacy firmware |
| `components/app_pax/` | libpax control + pax report task |
| `components/app_transport/` | outbound queue + transport task with pluggable sinks |
| `components/app_rcmd/` | remote command parser, legacy opcode table, console reader |
| `components/app_led/`, `app_button/`, `app_housekeeping/` | status LED, button, health checks |
| `components/libpax/` | vendored libpax (see `VENDORED.md`) |

## Tasks

| Task | Core | Prio | Job |
|---|---|---|---|
| transport | 0 | 2 | drains the payload queue into every sink |
| console | 0 | 2 | reads hex command lines from the USB serial port |
| pax | 1 | 4 | turns each libpax report into a counter payload |
| rcmd | 1 | 3 | executes remote commands one at a time |
| button | 1 | 5 | debounces the BOOT button (click = send count now, hold 1 s = button payload) |
| led | 1 | 1 | boot / run / error patterns, flash per sent payload |
| housekeep | 1 | 1 | heap, stack and queue health every 30 s |

WiFi, the BT controller and the libpax timers run on core 0 (ESP-IDF pins them
there). Tasks communicate only through queues, task notifications and the
mutex-guarded config; there are no shared mutable globals.

## Console commands

Type hex bytes and press Enter, using the same opcodes as the LoRaWAN downlink:

| Send | Effect |
|---|---|
| `80` | get config (reply on port 3) |
| `81` | get status (reply on port 2) |
| `0a 0f` | set send cycle to 30 s |
| `0e 01` | BLE counting on |
| `01 50` | RSSI limit -80 dBm |
| `21` | save config to flash |
| `09 02` | factory reset and restart |

Payloads are printed as `payload: port=1 len=2 hex=0700`.

Payload byte layouts match the legacy firmware, with one change: the reset
reason byte in the status payload (port 2) is now a `hal_reset_reason_t` value
(see `components/app_hal/include/hal_sys.h`), not an ESP32 ROM reset code.

## Status

Done: WiFi + BLE counting, plain/packed payloads, the full legacy opcode table,
NVS config, console transport, LED, button, watchdogs, host tests.
Not yet ported: LoRaWAN, display, GPS, sensors, SD card, battery, OTA, deep sleep.
On this board the related opcodes are accepted and logged as unavailable.
MQTT over WiFi cannot coexist with WiFi sniffing on one radio.

# License

Copyright 2018-2022 Oliver Brandmueller <ob@sysadm.in>

Copyright 2018-2022 Klaus Wilting <verkehrsrot@arcor.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE:
Parts of the source files in this repository are made available under different licenses,
see file <A HREF="https://github.com/cyberman54/ESP32-Paxcounter/blob/master/LICENSE">LICENSE.txt</A> in this repository. Refer to each individual source file for more details.

# Credits

Thanks to
- [Oliver Brandmüller](https://github.com/spmrider) for idea and initial setup of this project
- [Charles Hallard](https://github.com/hallard) for major code contributions to this project
- [robbi5](https://github.com/robbi5) for the payload converter
- [Caspar Armster](https://www.dasdigidings.de/) for the The Things Stack V3 payload converter
- [terrillmoore](https://github.com/mcci-catena) for maintaining the LMIC for arduino LoRaWAN stack
- [sbamueller](https://github.com/sbamueller) for writing the tutorial in Make Magazine
- [Stefan](https://github.com/nerdyscout) for paxcounter opensensebox integration
- [August Quint](https://github.com/AugustQu) for adding SD card data logger and SDS011 support
- [t-huyeng](https://github.com/t-huyeng) for adding a CI workflow and rework documentation
- [TD-er](https://github.com/TD-er) for bugfixings and T-Beam documentation
