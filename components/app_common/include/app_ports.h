// app_ports.h - payload port numbers (LoRaWAN FPort compatible with the
// legacy firmware, so existing TTN/Helium decoders keep working).

#pragma once

#define APP_PORT_COUNTER   1   // counts
#define APP_PORT_RCMD      2   // remote commands (downlink)
#define APP_PORT_STATUS    2   // remote command results
#define APP_PORT_CONFIG    3   // config query results
#define APP_PORT_GPS       4
#define APP_PORT_BUTTON    5   // button pressed signal
#define APP_PORT_BME       7
#define APP_PORT_BATT      8
#define APP_PORT_TIME      9   // time query and response
#define APP_PORT_SENSOR1   10
#define APP_PORT_SENSOR2   11
#define APP_PORT_SENSOR3   12
