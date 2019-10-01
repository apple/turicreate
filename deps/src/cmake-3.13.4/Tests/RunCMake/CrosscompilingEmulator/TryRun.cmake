set(CMAKE_CROSSCOMPILING 1)

try_run(run_result compile_result
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_src_exiterror.cxx
  RUN_OUTPUT_VARIABLE run_output)

message(STATUS "run_output: ${run_output}")
message(STATUS "run_result: ${run_result}")

set(CMAKE_CROSSCOMPILING_EMULATOR ${CMAKE_CROSSCOMPILING_EMULATOR}
  --flag
  "multi arg")
try_run(run_result compile_result
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_src_exiterror.cxx
  RUN_OUTPUT_VARIABLE run_output)
message(STATUS "Emulator with arguments run_output: ${run_output}")
