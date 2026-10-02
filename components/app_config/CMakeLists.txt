# Device runtime configuration.
#   config_rules.cpp - pure defaults/validation (host-testable)
#   config_store.cpp - NVS persistence + mutex-guarded access
idf_component_register(SRCS "config_rules.cpp" "config_store.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal
                       PRIV_REQUIRES log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
