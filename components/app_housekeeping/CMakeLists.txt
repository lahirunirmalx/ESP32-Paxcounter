# Periodic health check: heap, task stacks, transport drops.
idf_component_register(SRCS "housekeeping.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal
                       PRIV_REQUIRES app_common app_transport log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
