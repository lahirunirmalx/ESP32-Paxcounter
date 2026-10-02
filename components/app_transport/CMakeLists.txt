# Outbound payload queue + transport task with pluggable sinks.
idf_component_register(SRCS "transport.cpp" "sink_console.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_common app_hal
                       PRIV_REQUIRES log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
