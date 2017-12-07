
enable_language(CXX)

get_property(configs GLOBAL PROPERTY DEBUG_CONFIGURATIONS)
message("CONFIGS:${configs}")

add_library(iface1 INTERFACE)
target_link_libraries(iface1 INTERFACE debug external1)

get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")

set_property(GLOBAL APPEND PROPERTY DEBUG_CONFIGURATIONS EXTRA)
get_property(configs GLOBAL PROPERTY DEBUG_CONFIGURATIONS)
message("CONFIGS:${configs}")

get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")
target_link_libraries(iface1 INTERFACE debug external2)
get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS NEW CONFIGS)
get_property(configs GLOBAL PROPERTY DEBUG_CONFIGURATIONS)
message("CONFIGS:${configs}")

get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")
target_link_libraries(iface1 INTERFACE debug external3)
get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")

set_property(GLOBAL APPEND PROPERTY DEBUG_CONFIGURATIONS EXTRA)
get_property(configs GLOBAL PROPERTY DEBUG_CONFIGURATIONS)
message("CONFIGS:${configs}")

get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")
target_link_libraries(iface1 INTERFACE debug external4)
get_property(tgt_iface TARGET iface1 PROPERTY INTERFACE_LINK_LIBRARIES)
message("IFACE1:${tgt_iface}")
