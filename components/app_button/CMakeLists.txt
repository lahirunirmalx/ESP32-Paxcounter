# Button task: GPIO interrupt -> task notification -> debounce -> action.
idf_component_register(SRCS "button.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal
                       PRIV_REQUIRES app_board app_common app_config app_pax
                                     app_payload app_transport log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
