# Vendored libpax (WiFi + BLE MAC counting).
# Upstream: https://github.com/dbinfrago/libpax  commit 95758df72ad7981afdff76d1b7f06d4593c77345
# Local changes: explicit source list, LTO removed (cross-component LTO is
# not supported by the ESP-IDF build), feature macros come from platformio.ini.
idf_component_register(SRCS "libpax.cpp" "libpax_api.cpp" "wifiscan.cpp"
                            "blescan.cpp" "bt_hci_common.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash bt spi_flash esp_wifi esp_coex)
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -Wno-unused-variable)
