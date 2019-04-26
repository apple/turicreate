
enable_language(C)

set(obj "${CMAKE_C_OUTPUT_EXTENSION}")
if(BORLAND)
  set(pre -)
endif()

add_link_options($<$<AND:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>,$<CONFIG:Release>>:${pre}BADFLAG_SHARED_RELEASE${obj}>)
add_link_options($<$<AND:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>,$<CONFIG:Release>>:${pre}BADFLAG_MODULE_RELEASE${obj}>)
add_link_options($<$<AND:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>,$<CONFIG:Release>>:${pre}BADFLAG_EXECUTABLE_RELEASE${obj}>)

add_library(LinkOptions_shared SHARED LinkOptionsLib.c)

add_library(LinkOptions_mod MODULE LinkOptionsLib.c)

add_executable(LinkOptions_exe LinkOptionsExe.c)
