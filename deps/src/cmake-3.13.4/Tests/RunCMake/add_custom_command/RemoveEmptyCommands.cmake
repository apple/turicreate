enable_language(CXX)

# reduce number of configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(main_file "${CMAKE_BINARY_DIR}/main.cpp")
file(WRITE "${main_file}" "test")
add_executable(exe "${main_file}")

# add one command for all and one for debug only
add_custom_command(TARGET exe
  COMMAND "cmd_1" "cmd_1_arg"
  COMMAND $<$<CONFIG:Debug>:cmd_1_dbg> $<$<CONFIG:Debug>:cmd_1_dbg_arg>)

# add command for debug only
add_custom_command(TARGET exe
  COMMAND $<$<CONFIG:Debug>:cmd_2_dbg> $<$<CONFIG:Debug>:cmd_2_dbg_arg>)

# add separate commands for configurations
add_custom_command(TARGET exe
  COMMAND $<$<CONFIG:Debug>:cmd_3_dbg> $<$<CONFIG:Debug>:cmd_3_dbg_arg>
  COMMAND $<$<CONFIG:Release>:cmd_3_rel> $<$<CONFIG:Release>:cmd_3_rel_arg>)
