# Pax counting: libpax control + report task that turns counts into payloads.
idf_component_register(SRCS "pax.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal
                       PRIV_REQUIRES libpax app_config app_payload app_transport
                                     app_common log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
