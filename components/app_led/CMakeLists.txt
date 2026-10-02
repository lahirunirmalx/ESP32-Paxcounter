# Status LED task.
idf_component_register(SRCS "led.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal app_transport
                       PRIV_REQUIRES app_board app_common log)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
