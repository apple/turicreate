
add_library(iface INTERFACE)
install(TARGETS iface EXPORT ifaceExport)

export(EXPORT ifaceExport FILE "${CMAKE_CURRENT_BINARY_DIR}/theTargets.cmake")
include("${CMAKE_CURRENT_BINARY_DIR}/theTargets.cmake")
