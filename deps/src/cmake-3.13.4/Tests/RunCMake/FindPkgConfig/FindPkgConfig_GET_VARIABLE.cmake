find_package(PkgConfig REQUIRED)
pkg_check_modules(GOBJECT_INTROSPECTION QUIET gobject-introspection-1.0)

if (GOBJECT_INTROSPECTION_FOUND)
  pkg_get_variable(g_ir_scanner gobject-introspection-1.0 g_ir_scanner)
  message(STATUS "g_ir_scanner: ${g_ir_scanner}")
else ()
  message(STATUS "g_ir_scanner: skipping test; gobject-introspection-1.0 not found /g-ir-scanner")
endif ()
