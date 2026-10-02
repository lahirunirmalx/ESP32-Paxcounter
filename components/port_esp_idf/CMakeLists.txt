# ESP-IDF implementation of the app_hal contract.
# This is the ONLY component allowed to include driver/*.h.
idf_component_register(SRCS "port_gpio.cpp"
                            "port_uart.cpp"
                            "port_kv.cpp"
                            "port_sys.cpp"
                       PRIV_INCLUDE_DIRS "."
                       REQUIRES app_hal
                       PRIV_REQUIRES driver esp_driver_gpio esp_driver_uart
                                     nvs_flash esp_system esp_hw_support
                                     esp_timer log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
