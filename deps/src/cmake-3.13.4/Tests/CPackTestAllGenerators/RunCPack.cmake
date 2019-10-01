if(NOT DEFINED dir)
  message(FATAL_ERROR "dir not defined")
endif()

# Analyze 'cpack --help' output for list of available generators:
#
execute_process(COMMAND ${CMAKE_CPACK_COMMAND} --help
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
  WORKING_DIRECTORY ${dir})

string(REPLACE ";" "\\;" stdout "${stdout}")
string(REPLACE "\n" "E;" stdout "${stdout}")

set(collecting 0)
set(generators)
foreach(eline ${stdout})
  string(REGEX REPLACE "^(.*)E$" "\\1" line "${eline}")
  if(collecting AND NOT line STREQUAL "")
    string(REGEX REPLACE "^  ([^ ]+) += (.*)$" "\\1" gen "${line}")
    string(REGEX REPLACE "^  ([^ ]+) += (.*)$" "\\2" doc "${line}")
    set(generators ${generators} ${gen})
  endif()
  if(line STREQUAL "Generators")
    set(collecting 1)
  endif()
endforeach()

# Call cpack with -G on each available generator. We do not care if this
# succeeds or not. We expect it *not* to succeed if the underlying packaging
# tools are not installed on the system... This test is here simply to add
# coverage for the various cpack generators, even/especially to test ones
# where the tools are not installed.
#
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")

message(STATUS "CPack generators='${generators}'")

foreach(g ${generators})
  message(STATUS "Calling cpack -G ${g}...")
  execute_process(COMMAND ${CMAKE_CPACK_COMMAND} -G ${g}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    WORKING_DIRECTORY ${dir})
  message(STATUS "result='${result}'")
  message(STATUS "stdout='${stdout}'")
  message(STATUS "stderr='${stderr}'")
  message(STATUS "")
endforeach()
