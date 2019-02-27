
include(GenerateExportHeader)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

cmake_policy(SET CMP0022 NEW)
add_library(cmp0022lib SHARED cmp0022lib.cpp)
generate_export_header(cmp0022lib)
add_library(cmp0022ifacelib SHARED cmp0022ifacelib.cpp)
generate_export_header(cmp0022ifacelib)
target_link_libraries(cmp0022lib LINK_PUBLIC cmp0022ifacelib)

assert_property(cmp0022lib LINK_INTERFACE_LIBRARIES "")
assert_property(cmp0022ifacelib LINK_INTERFACE_LIBRARIES "")
assert_property(cmp0022lib INTERFACE_LINK_LIBRARIES "cmp0022ifacelib")
assert_property(cmp0022ifacelib INTERFACE_LINK_LIBRARIES "")

add_executable(cmp0022exe cmp0022exe.cpp)
target_link_libraries(cmp0022exe cmp0022lib)

add_library(staticlib1 STATIC staticlib1.cpp)
generate_export_header(staticlib1)
add_library(staticlib2 STATIC staticlib2.cpp)
generate_export_header(staticlib2)
target_link_libraries(staticlib1 LINK_PUBLIC staticlib2)

# Try adding a private link item to be propagated out of a static lib.
set(private_link "")
if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
  if (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    set(private_link "-Wl,-V")
  else()
    set(private_link "-Wl,-v")
  endif()
endif()
if(private_link)
  target_link_libraries(staticlib1 LINK_PRIVATE "${private_link}")
endif()

add_executable(staticlib_exe staticlib_exe.cpp)
target_link_libraries(staticlib_exe staticlib1)

add_library(onlyplainlib1 SHARED onlyplainlib1.cpp)
add_library(onlyplainlib2 SHARED onlyplainlib2.cpp)
target_link_libraries(onlyplainlib2 onlyplainlib1)

add_executable(onlyplainlib_user onlyplainlib_user.cpp)
target_link_libraries(onlyplainlib_user onlyplainlib2)
