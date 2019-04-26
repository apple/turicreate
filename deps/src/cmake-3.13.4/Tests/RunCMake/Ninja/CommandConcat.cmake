cmake_minimum_required(VERSION 3.7)
project(concat_cmd NONE)
set(output1 ${CMAKE_BINARY_DIR}/out1.txt)
set(output2 ${CMAKE_BINARY_DIR}/out2.txt)
file(REMOVE ${output1} ${output2})
# Check that second command runs if first command contains "||" which has higher precedence than "&&" on Windows
add_custom_target(concat_cmd ALL
  COMMAND ${CMAKE_COMMAND} -E echo "Hello || pipe world" && ${CMAKE_COMMAND} -E touch ${output1} || exit 1
  COMMAND ${CMAKE_COMMAND} -E touch ${output2})
# Check output
add_custom_target(check_output ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${output1} ${output1}.copy
  COMMAND ${CMAKE_COMMAND} -E copy ${output2} ${output2}.copy)
add_dependencies(check_output concat_cmd)
