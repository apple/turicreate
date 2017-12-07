add_library(iface INTERFACE)
add_custom_target(check ALL COMMAND echo
  $<TARGET_FILE:iface>
  $<TARGET_SONAME_FILE:iface>
  $<TARGET_LINKER_FILE:iface>
  )
