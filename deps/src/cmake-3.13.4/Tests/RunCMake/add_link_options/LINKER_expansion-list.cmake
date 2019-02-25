
enable_language(C)

add_executable(dump dump.c)

add_link_options("${LINKER_OPTION}")

# ensure no temp file will be used
string(REPLACE "${CMAKE_START_TEMP_FILE}" "" CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")
string(REPLACE "${CMAKE_END_TEMP_FILE}" "" CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")

add_library(example SHARED LinkOptionsLib.c)
# use LAUNCH facility to dump linker command
set_property(TARGET example PROPERTY RULE_LAUNCH_LINK "\"${CMAKE_CURRENT_BINARY_DIR}/dump${CMAKE_EXECUTABLE_SUFFIX}\"")

add_dependencies (example dump)

# generate reference for LINKER flag
if (CMAKE_C_LINKER_WRAPPER_FLAG)
  set(linker_flag ${CMAKE_C_LINKER_WRAPPER_FLAG})
  list(GET linker_flag -1 linker_space)
  if (linker_space STREQUAL " ")
    list(REMOVE_AT linker_flag -1)
  else()
    set(linker_space)
  endif()
  list (JOIN linker_flag " " linker_flag)
  if (CMAKE_C_LINKER_WRAPPER_FLAG_SEP)
    string (APPEND  linker_flag "${linker_space}" "-foo${CMAKE_C_LINKER_WRAPPER_FLAG_SEP}bar")
  else()
    set (linker_flag "${linker_flag}${linker_space}-foo ${linker_flag}${linker_space}bar")
  endif()
else()
  set(linker_flag "-foo bar")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LINKER.txt" "${linker_flag}")
