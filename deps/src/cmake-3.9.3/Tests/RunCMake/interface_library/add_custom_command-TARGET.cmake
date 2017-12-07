
add_library(iface INTERFACE)

add_custom_command(TARGET iface
  COMMAND "${CMAKE_COMMAND}" -E echo test
)
