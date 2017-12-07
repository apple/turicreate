
cmake_minimum_required(VERSION 2.8)

project(InterfaceLibrary)

add_library(iface_nodepends INTERFACE)
target_compile_definitions(iface_nodepends INTERFACE IFACE_DEFINE)

add_subdirectory(headerdir)

# Add an interface target in a subdirectory that uses an imported interface.
add_subdirectory(ifacedir)

# Poison an imported interface with the same name as that in the subdir
# to ensure that the transitive lookup occurs in the subdir.
add_library(imp::iface INTERFACE IMPORTED)
set_property(TARGET imp::iface APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SOMEPROP)
set_property(TARGET imp::iface PROPERTY INTERFACE_SOMEPROP OFF)
set_property(TARGET imp::iface PROPERTY INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/does_not_exist.cpp)

add_library(objlib OBJECT obj.cpp)
add_library(iface_objlib INTERFACE)
target_sources(iface_objlib INTERFACE $<TARGET_OBJECTS:objlib>)

add_library(intermediate INTERFACE)
target_link_libraries(intermediate INTERFACE iface_objlib)

add_library(item_fake_tgt STATIC item_fake.cpp)
set_property(TARGET item_fake_tgt PROPERTY OUTPUT_NAME item_fake)
add_library(item_real STATIC item.cpp)
add_library(item_iface INTERFACE IMPORTED)
set_property(TARGET item_iface PROPERTY IMPORTED_LIBNAME item_real)
add_dependencies(item_iface item_real)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(InterfaceLibrary definetestexe.cpp)
target_link_libraries(InterfaceLibrary
  iface_nodepends
  headeriface
  subiface
  intermediate

  item_iface
  item_fake # ensure that 'item_real' is ordered in place of item_iface
  )
add_dependencies(InterfaceLibrary item_fake_tgt)

add_subdirectory(libsdir)

add_executable(sharedlibtestexe sharedlibtestexe.cpp)
target_link_libraries(sharedlibtestexe shared_iface imported::iface)

add_library(broken EXCLUDE_FROM_ALL broken.cpp)

add_library(iface_broken INTERFACE)
# This is not a dependency, so broken will not be built (and the error in
# it will not be hit)
target_link_libraries(iface_broken INTERFACE broken)

add_library(iface_whitelist INTERFACE)
# The target property CUSTOM will never be evaluated on the INTERFACE library.
target_link_libraries(iface_whitelist INTERFACE $<$<BOOL:$<TARGET_PROPERTY:CUSTOM>>:irrelevant>)

add_executable(exec_whitelist dummy.cpp)
target_link_libraries(exec_whitelist iface_whitelist)

set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)
add_library(iface_imported INTERFACE IMPORTED)
set_property(TARGET iface_imported PROPERTY
  INTERFACE_COMPILE_DEFINITIONS
    $<$<CONFIG:SPECIAL>:SPECIAL_MODE>
    $<$<CONFIG:Debug>:DEBUG_MODE>
)
set_property(TARGET iface_imported PROPERTY
  MAP_IMPORTED_CONFIG_DEBUG SPECIAL
)

add_executable(map_config map_config.cpp)
target_link_libraries(map_config iface_imported)
