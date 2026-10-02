idf_component_register(SRCS "app_main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES app_hal app_board app_common app_config app_transport
                                app_pax app_rcmd app_led app_button app_housekeeping
                                esp_event log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
