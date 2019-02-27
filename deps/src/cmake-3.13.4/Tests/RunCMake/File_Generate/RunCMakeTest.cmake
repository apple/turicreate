include(RunCMake)

run_cmake(CMP0070-NEW)
run_cmake(CMP0070-OLD)
run_cmake(CMP0070-WARN)

run_cmake(CommandConflict)
if(RunCMake_GENERATOR_IS_MULTI_CONFIG)
  run_cmake(OutputConflict)
endif()
run_cmake(EmptyCondition1)
run_cmake(EmptyCondition2)
run_cmake(BadCondition)
run_cmake(DebugEvaluate)
run_cmake(GenerateSource)
run_cmake(OutputNameMatchesSources)
run_cmake(OutputNameMatchesObjects)
run_cmake(OutputNameMatchesOtherSources)
file(READ "${RunCMake_BINARY_DIR}/OutputNameMatchesOtherSources-build/1somefile.cpp" file_contents)
if (NOT file_contents MATCHES "generated.cpp.rule")
  message(SEND_ERROR "Rule file not in target sources! ${file_contents}")
endif()

run_cmake(COMPILE_LANGUAGE-genex)
foreach(l CXX C)
  file(READ "${RunCMake_BINARY_DIR}/COMPILE_LANGUAGE-genex-build/opts-${l}.txt" l_defs)
  if (NOT l_defs STREQUAL "LANG_IS_${l}\n")
    message(FATAL_ERROR "File content does not match: ${l_defs}")
  endif()
endforeach()

set(timeformat "%Y%j%H%M%S")

file(REMOVE "${RunCMake_BINARY_DIR}/WriteIfDifferent-build/output_file.txt")
set(RunCMake_TEST_OPTIONS "-DTEST_FILE=WriteIfDifferent.cmake")
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/WriteIfDifferent-build")
run_cmake(WriteIfDifferent-prepare)
unset(RunCMake_TEST_OPTIONS)
unset(RunCMake_TEST_BINARY_DIR)
file(TIMESTAMP "${RunCMake_BINARY_DIR}/WriteIfDifferent-build/output_file.txt" timestamp ${timeformat})
if(NOT timestamp)
  message(SEND_ERROR "Could not get timestamp for \"${RunCMake_BINARY_DIR}/WriteIfDifferent-build/output_file.txt\"")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)

set(RunCMake_TEST_NO_CLEAN ON)
run_cmake(WriteIfDifferent)
file(TIMESTAMP "${RunCMake_BINARY_DIR}/WriteIfDifferent-build/output_file.txt" timestamp_after ${timeformat})
if(NOT timestamp_after)
  message(SEND_ERROR "Could not get timestamp for \"${RunCMake_BINARY_DIR}/WriteIfDifferent-build/output_file.txt\"")
endif()
unset(RunCMake_TEST_NO_CLEAN)

if (NOT timestamp_after STREQUAL timestamp)
  message(SEND_ERROR "WriteIfDifferent changed output file.")
endif()

if (UNIX AND EXISTS /bin/sh)
  set(RunCMake_TEST_NO_CLEAN ON)
  run_cmake(CarryPermissions)
  execute_process(
    COMMAND "${RunCMake_BINARY_DIR}/CarryPermissions-build/output_script.sh"
    OUTPUT_VARIABLE script_output
    RESULT_VARIABLE script_result
    ERROR_VARIABLE script_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (script_result)
    message(SEND_ERROR "Generated script did not execute correctly: [${script_result}]\n${script_output}\n====\n${script_error}")
  endif()
  if (NOT script_output STREQUAL SUCCESS)
    message(SEND_ERROR "Generated script did not execute correctly:\n${script_output}\n====\n${script_error}")
  endif()
endif()

if (RunCMake_GENERATOR MATCHES Makefiles)
  file(MAKE_DIRECTORY "${RunCMake_BINARY_DIR}/ReRunCMake-build/")
  file(WRITE "${RunCMake_BINARY_DIR}/ReRunCMake-build/input_file.txt" "InitialContent\n")

  set(RunCMake_TEST_NO_CLEAN ON)
  run_cmake(ReRunCMake)
  unset(RunCMake_TEST_NO_CLEAN)
  file(TIMESTAMP "${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt" timestamp ${timeformat})
  if(NOT timestamp)
    message(SEND_ERROR "Could not get timestamp for \"${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt\"")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)

  file(WRITE "${RunCMake_BINARY_DIR}/ReRunCMake-build/input_file.txt" "ChangedContent\n")
  execute_process(COMMAND ${CMAKE_COMMAND} --build "${RunCMake_BINARY_DIR}/ReRunCMake-build/")
  file(READ "${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt" out_content)

  if(NOT out_content STREQUAL "ChangedContent\n")
    message(SEND_ERROR "File did not change: \"${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt\"")
  endif()


  file(REMOVE "${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt")
  execute_process(COMMAND ${CMAKE_COMMAND} --build "${RunCMake_BINARY_DIR}/ReRunCMake-build/")

  if (NOT EXISTS "${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt")
    message(SEND_ERROR "File did not re-generate: \"${RunCMake_BINARY_DIR}/ReRunCMake-build/output_file.txt\"")
  endif()
endif()
