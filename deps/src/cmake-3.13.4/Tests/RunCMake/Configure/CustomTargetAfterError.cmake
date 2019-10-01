message(SEND_ERROR "Error before add_custom_target")
add_custom_target(foo COMMAND echo)
message(SEND_ERROR "Error after add_custom_target")
