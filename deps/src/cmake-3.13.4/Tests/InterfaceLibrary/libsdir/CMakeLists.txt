
include(GenerateExportHeader)

add_library(sharedlib SHARED sharedlib.cpp)
generate_export_header(sharedlib)

add_library(shareddependlib SHARED shareddependlib.cpp)
generate_export_header(shareddependlib)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

target_link_libraries(sharedlib PUBLIC shareddependlib)

target_include_directories(shareddependlib
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/shareddependlib")
target_compile_definitions(shareddependlib
  INTERFACE $<1:SHAREDDEPENDLIB_DEFINE>)

target_include_directories(sharedlib
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sharedlib")
target_compile_definitions(shareddependlib
  INTERFACE $<1:SHAREDLIB_DEFINE>)

add_library(shared_iface INTERFACE)
target_link_libraries(shared_iface INTERFACE sharedlib)

add_library(imported::iface INTERFACE IMPORTED GLOBAL)
