# Remote commands.
#   rcmd_parser.cpp   - pure opcode/hex parsing (host-testable)
#   rcmd_handlers.cpp - legacy opcode table bound to config/pax/transport
#   rcmd_task.cpp     - command queue task + console line reader task
idf_component_register(SRCS "rcmd_parser.cpp" "rcmd_handlers.cpp" "rcmd_task.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES app_hal
                       PRIV_REQUIRES app_board app_common app_config app_pax
                                     app_payload app_transport log newlib)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
