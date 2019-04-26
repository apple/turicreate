cmake_minimum_required (VERSION 3.9)
project(PrecompiledHeader C)

# Make sure the proper compiler is in use.
if(NOT MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Intel")
  message(FATAL_ERROR "The PrecompiledHeader test works only with MSVC or Intel")
endif()

# Compute a custom name for the precompiled header.
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  set(PCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/PCH/${CMAKE_CFG_INTDIR}")
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/PCH/${cfg})
  endforeach()
else()
  set(PCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/PCH")
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/PCH)
endif()

set(PCH_USE_INCLUDE_DIR 0)
set(PCH_FILE "\"/Fp${PCH_DIR}/foo_precompiled.pch\"")

# Choose between an explicit include path and using /I during
# precompilation.  The /I form is used to test that the PCH is
# actually used.  In practice the include path form would be used.
if(PCH_USE_INCLUDE_DIR)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
  set(PCH_INCLUDE_DIR "\"/I${CMAKE_CURRENT_SOURCE_DIR}/include\"")
endif()

# Create a target that will use a precompiled header.
set(foo_SRCS foo1.c foo2.c)
add_executable(foo foo_precompile.c ${foo_SRCS})

# Setup flags on the target to create and use the precompiled header.
set_target_properties(foo PROPERTIES
  COMPILE_FLAGS "/Yufoo_precompiled.h /FIfoo_precompiled.h ${PCH_FILE}")
set_source_files_properties(foo_precompile.c PROPERTIES
  COMPILE_FLAGS "/Ycfoo_precompiled.h ${PCH_INCLUDE_DIR}")

# Setup dependencies for precompiled header creation and use.  The VS
# IDE takes care of this automatically.
if("${CMAKE_GENERATOR}" MATCHES "Makefile" OR
   "${CMAKE_GENERATOR}" MATCHES "Ninja")
  # This source file creates the precompiled header as a side-effect.
  set_source_files_properties(foo_precompile.c PROPERTIES
    OBJECT_OUTPUTS "${PCH_DIR}/foo_precompiled.pch")

  # These source files use the precompiled header.
  set_source_files_properties(${foo_SRCS} PROPERTIES
    OBJECT_DEPENDS "${PCH_DIR}/foo_precompiled.pch")
endif()
