
add_library(iface INTERFACE)

set_property(TARGET iface PROPERTY OUTPUT_NAME output)
set_property(TARGET iface APPEND PROPERTY OUTPUT_NAME append)
get_target_property(outname iface OUTPUT_NAME)

# Properties starting with `_` are allowed.
set_property(TARGET iface PROPERTY "_custom_property" output)
set_property(TARGET iface APPEND PROPERTY "_custom_property" append)
get_target_property(outname iface "_custom_property")

# Properties starting with a lowercase letter are allowed.
set_property(TARGET iface PROPERTY "custom_property" output)
set_property(TARGET iface APPEND PROPERTY "custom_property" append)
get_target_property(outname iface "custom_property")
