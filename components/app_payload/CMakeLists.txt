# Payload encoder. Pure C++ (host-testable), no platform dependencies.
idf_component_register(SRCS "payload.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_config app_common)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra -Wconversion)
