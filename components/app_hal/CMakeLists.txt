# Vendor-neutral HAL contract. Headers must not include any ESP-IDF header.
idf_component_register(SRCS "hal_result.c"
                       INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
