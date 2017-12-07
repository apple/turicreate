
add_library(iface INTERFACE)

set_property(TARGET iface PROPERTY OUTPUT_NAME output)
set_property(TARGET iface APPEND PROPERTY OUTPUT_NAME append)
get_target_property(outname iface OUTPUT_NAME)
