set(CMAKE_STATIC_LINKER_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG${obj})
add_library(LinkFlags_lib_flags_config STATIC ../LinkFlagsLib.c)

set(CMAKE_SHARED_LINKER_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG${obj})
add_library(LinkFlags_dll_flags_config SHARED ../LinkFlagsLib.c)

set(CMAKE_MODULE_LINKER_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG${obj})
add_library(LinkFlags_mod_flags_config MODULE ../LinkFlagsLib.c)

set(CMAKE_EXE_LINKER_FLAGS_${TEST_CONFIG_UPPER} ${pre}BADFLAG${obj})
add_executable(LinkFlags_exe_flags_config ../LinkFlagsExe.c)
