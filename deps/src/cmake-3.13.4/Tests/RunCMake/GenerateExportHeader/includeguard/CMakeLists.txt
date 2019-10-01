set(libincludeguard_SRC libincludeguard.cpp)

add_library(includeguard_macro ${libincludeguard_SRC})
generate_export_header(includeguard_macro)

set(EXPORT_HEADER includeguard_macro_export.h)
set(DEF_SYMBOL INCLUDEGUARD_MACRO_EXPORT_H)
set(NDEF_SYMBOL CUSTOM_GUARD)
configure_file(main.cpp.in main_macro.cpp)
add_executable(test_includeguard_macro ${CMAKE_CURRENT_BINARY_DIR}/main_macro.cpp)

add_library(includeguard_custom ${libincludeguard_SRC})
generate_export_header(includeguard_custom INCLUDE_GUARD_NAME CUSTOM_GUARD)

set(EXPORT_HEADER includeguard_custom_export.h)
set(DEF_SYMBOL CUSTOM_GUARD)
set(NDEF_SYMBOL INCLUDEGUARD_CUSTOM_EXPORT_H)
configure_file(main.cpp.in main_custom.cpp)
add_executable(test_includeguard_custom ${CMAKE_CURRENT_BINARY_DIR}/main_custom.cpp)
