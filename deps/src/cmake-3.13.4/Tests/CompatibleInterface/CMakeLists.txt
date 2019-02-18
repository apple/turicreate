
cmake_minimum_required(VERSION 3.0)

project(CompatibleInterface)

include(GenerateExportHeader)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(iface1 INTERFACE)
set_property(TARGET iface1 APPEND PROPERTY
  COMPATIBLE_INTERFACE_BOOL
    BOOL_PROP1
    BOOL_PROP2
    BOOL_PROP3
    BOOL_PROP4
)
set_property(TARGET iface1 APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING
    STRING_PROP1
    STRING_PROP2
    STRING_PROP3
)
set_property(TARGET iface1 APPEND PROPERTY
  COMPATIBLE_INTERFACE_NUMBER_MIN
    NUMBER_MIN_PROP1
    NUMBER_MIN_PROP2
    NUMBER_MIN_PROP3
    NUMBER_MIN_PROP4
)
set_property(TARGET iface1 APPEND PROPERTY
  COMPATIBLE_INTERFACE_NUMBER_MAX
    NUMBER_MAX_PROP1
    NUMBER_MAX_PROP2
)

set(CMAKE_DEBUG_TARGET_PROPERTIES
  BOOL_PROP1 BOOL_PROP2 BOOL_PROP3 BOOL_PROP4
  STRING_PROP1 STRING_PROP2 STRING_PROP3
  NUMBER_MIN_PROP1 NUMBER_MIN_PROP2 NUMBER_MIN_PROP3 NUMBER_MIN_PROP4
  NUMBER_MAX_PROP1 NUMBER_MAX_PROP2
)

set_property(TARGET iface1 PROPERTY INTERFACE_BOOL_PROP1 ON)
set_property(TARGET iface1 PROPERTY INTERFACE_BOOL_PROP2 ON)
set_property(TARGET iface1 PROPERTY INTERFACE_STRING_PROP1 prop1)
set_property(TARGET iface1 PROPERTY INTERFACE_STRING_PROP2 prop2)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MIN_PROP1 100)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MIN_PROP2 200)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MIN_PROP3 0x10)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MIN_PROP4 0x10)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MAX_PROP1 100)
set_property(TARGET iface1 PROPERTY INTERFACE_NUMBER_MAX_PROP2 200)

add_executable(CompatibleInterface main.cpp)
target_link_libraries(CompatibleInterface iface1)

add_library(foo STATIC foo.cpp)
add_library(bar SHARED bar.cpp)
set_property(TARGET foo APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SOMEPROP)
set_property(TARGET foo PROPERTY INTERFACE_SOMEPROP ON)
# Use LINK_ONLY to suppress usage requirements and allow the check to pass.
set_property(TARGET bar PROPERTY INTERFACE_LINK_LIBRARIES $<LINK_ONLY:foo>)
set_property(TARGET CompatibleInterface PROPERTY SOMEPROP OFF)
target_link_libraries(CompatibleInterface bar)

set_property(TARGET CompatibleInterface PROPERTY BOOL_PROP2 ON)
set_property(TARGET CompatibleInterface PROPERTY BOOL_PROP3 ON)
set_property(TARGET CompatibleInterface PROPERTY STRING_PROP2 prop2)
set_property(TARGET CompatibleInterface PROPERTY STRING_PROP3 prop3)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MIN_PROP1 50)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MIN_PROP2 250)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MIN_PROP3 0xa)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MIN_PROP4 0x1A)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MAX_PROP1 50)
set_property(TARGET CompatibleInterface PROPERTY NUMBER_MAX_PROP2 250)

target_compile_definitions(CompatibleInterface
  PRIVATE
    $<$<BOOL:$<TARGET_PROPERTY:BOOL_PROP1>>:BOOL_PROP1>
    $<$<BOOL:$<TARGET_PROPERTY:BOOL_PROP2>>:BOOL_PROP2>
    $<$<BOOL:$<TARGET_PROPERTY:BOOL_PROP3>>:BOOL_PROP3>
    $<$<STREQUAL:$<TARGET_PROPERTY:STRING_PROP1>,prop1>:STRING_PROP1>
    $<$<STREQUAL:$<TARGET_PROPERTY:STRING_PROP2>,prop2>:STRING_PROP2>
    $<$<STREQUAL:$<TARGET_PROPERTY:STRING_PROP3>,prop3>:STRING_PROP3>
    $<$<STREQUAL:$<TARGET_PROPERTY:NUMBER_MIN_PROP1>,50>:NUMBER_MIN_PROP1=50>
    $<$<STREQUAL:$<TARGET_PROPERTY:NUMBER_MIN_PROP2>,200>:NUMBER_MIN_PROP2=200>
    $<$<EQUAL:$<TARGET_PROPERTY:NUMBER_MIN_PROP3>,0xA>:NUMBER_MIN_PROP3=0xA>
    $<$<STREQUAL:$<TARGET_PROPERTY:NUMBER_MIN_PROP4>,0x10>:NUMBER_MIN_PROP4=0x10>
    $<$<STREQUAL:$<TARGET_PROPERTY:NUMBER_MAX_PROP1>,100>:NUMBER_MAX_PROP1=100>
    $<$<STREQUAL:$<TARGET_PROPERTY:NUMBER_MAX_PROP2>,250>:NUMBER_MAX_PROP2=250>
)


add_library(iface2 SHARED iface2.cpp)
generate_export_header(iface2)

set_property(TARGET iface2 APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING
    Iface2_PROP
)

# For the LINK_LIBRARIES and related properties, we should not evaluate
# properties defined only in the interface - they should be implicitly zero
set_property(TARGET iface2
  APPEND PROPERTY
    LINK_INTERFACE_LIBRARIES $<$<BOOL:$<TARGET_PROPERTY:BOOL_PROP4>>:nonexistent>
)
target_link_libraries(CompatibleInterface iface2
      $<$<BOOL:$<TARGET_PROPERTY:Iface2_PROP>>:nonexistent>
)
# Test that this does not segfault:
target_compile_definitions(CompatibleInterface
  PRIVATE
    $<$<BOOL:$<TARGET_PROPERTY:Iface2_PROP>>:SOME_DEFINE>
)

# The COMPATIBLE_INTERFACE_* properties are only read from dependencies
# in the interface. Populating it on the CompatibleInterface target does
# not have any effect on the interpretation of the INTERFACE variants
# in dependencies.
set_property(TARGET iface1 PROPERTY
  INTERFACE_NON_RELEVANT_PROP ON
)
set_property(TARGET iface2 PROPERTY
  INTERFACE_NON_RELEVANT_PROP ON
)
set_property(TARGET CompatibleInterface APPEND PROPERTY
  COMPATIBLE_INTERFACE_BOOL
    NON_RELEVANT_PROP
)
