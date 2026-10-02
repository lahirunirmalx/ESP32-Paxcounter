// board.h - pin map for a generic ESP32-WROOM-32 devkit (ESP32-DevKitC style).
//
// The only place GPIO numbers are written down. Feature components take
// their pins from here and talk to them through hal_*.

#pragma once

#define BOARD_NAME "esp32-wroom-devkit"

// On-board LED.
// STRAPPING: GPIO2 must be low or floating at reset to allow serial download.
// It is only driven as an output after boot, so this is safe.
#define BOARD_HAS_LED          1
#define BOARD_LED_PIN          2
#define BOARD_LED_ACTIVE_HIGH  1

// BOOT button.
// STRAPPING: GPIO0 low at reset selects download mode. The devkit has an
// external pull-up and the pin is only ever read as an input.
#define BOARD_HAS_BUTTON         1
#define BOARD_BUTTON_PIN         0
#define BOARD_BUTTON_ACTIVE_LOW  1

// USB-serial console (logs, payload output and remote commands).
#define BOARD_CONSOLE_UART  0
#define BOARD_CONSOLE_BAUD  115200
