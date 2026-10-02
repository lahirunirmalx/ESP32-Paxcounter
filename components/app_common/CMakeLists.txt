# Shared application definitions: task layout, message types, LoRaWAN-style
# port numbers and the task-watchdog helper.
idf_component_register(SRCS "app_wdt.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_system log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
