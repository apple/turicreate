cmake_minimum_required(VERSION 2.8)
project(LinkFlags C)

string(TOUPPER "${TEST_CONFIG}" TEST_CONFIG_UPPER)
set(obj "${CMAKE_C_OUTPUT_EXTENSION}")
if(BORLAND)
  set(pre -)
endif()

add_library(LinkFlags_lib STATIC LinkFlagsLib.c)
set_property(TARGET LinkFlags_lib PROPERTY STATIC_LIBRARY_FLAGS ${pre}BADFLAG${obj})

add_library(LinkFlags_dll SHARED LinkFlagsLib.c)
set_property(TARGET LinkFlags_dll PROPERTY LINK_FLAGS ${pre}BADFLAG${obj})

add_library(LinkFlags_mod MODULE LinkFlagsLib.c)
set_property(TARGET LinkFlags_mod PROPERTY LINK_FLAGS ${pre}BADFLAG${obj})

add_executable(LinkFlags_exe LinkFlagsExe.c)
set_property(TARGET LinkFlags_exe PROPERTY LINK_FLAGS ${pre}BADFLAG${obj})

add_library(LinkFlags_lib_config STATIC LinkFlagsLib.c)
set_property(TARGET LinkFlags_lib_config PROPERTY STATIC_LIBRARY_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG_${TEST_CONFIG}${obj})

add_library(LinkFlags_dll_config SHARED LinkFlagsLib.c)
set_property(TARGET LinkFlags_dll_config PROPERTY LINK_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG_${TEST_CONFIG}${obj})

add_library(LinkFlags_mod_config MODULE LinkFlagsLib.c)
set_property(TARGET LinkFlags_mod_config PROPERTY LINK_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG_${TEST_CONFIG}${obj})

add_executable(LinkFlags_exe_config LinkFlagsExe.c)
set_property(TARGET LinkFlags_exe_config PROPERTY LINK_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG_${TEST_CONFIG}${obj})

add_executable(LinkFlags LinkFlags.c)

add_subdirectory(LinkerFlags)
add_subdirectory(LinkerFlagsConfig)
