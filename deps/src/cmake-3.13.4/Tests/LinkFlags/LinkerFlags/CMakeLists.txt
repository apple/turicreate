set(CMAKE_STATIC_LINKER_FLAGS ${pre}BADFLAG${obj})
add_library(LinkFlags_lib_flags STATIC ../LinkFlagsLib.c)

set(CMAKE_SHARED_LINKER_FLAGS ${pre}BADFLAG${obj})
add_library(LinkFlags_dll_flags SHARED ../LinkFlagsLib.c)

set(CMAKE_MODULE_LINKER_FLAGS ${pre}BADFLAG${obj})
add_library(LinkFlags_mod_flags MODULE ../LinkFlagsLib.c)

set(CMAKE_EXE_LINKER_FLAGS ${pre}BADFLAG${obj})
add_executable(LinkFlags_exe_flags ../LinkFlagsExe.c)
