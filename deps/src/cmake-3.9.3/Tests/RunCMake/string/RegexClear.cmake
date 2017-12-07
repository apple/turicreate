cmake_minimum_required (VERSION 3.0)
project (RegexClear C)

function (output_results msg)
  message("results from: ${msg}")
  message("CMAKE_MATCH_0: -->${CMAKE_MATCH_0}<--")
  message("CMAKE_MATCH_1: -->${CMAKE_MATCH_1}<--")
  message("CMAKE_MATCH_2: -->${CMAKE_MATCH_2}<--")
  message("CMAKE_MATCH_COUNT: -->${CMAKE_MATCH_COUNT}<--")
endfunction ()

function (check_for_success msg)
  if (CMAKE_MATCH_1 STREQUAL "0" AND
      CMAKE_MATCH_2 STREQUAL "1")
    message("Matched string properly")
  else ()
    message("Failed to match properly")
  endif ()
  output_results("${msg}")
endfunction ()

function (check_for_failure msg)
  if (CMAKE_MATCH_1 STREQUAL "" AND
      CMAKE_MATCH_2 STREQUAL "")
    message("Matched nothing properly")
  else ()
    message("Found a match where there should be none")
  endif ()
  output_results("${msg}")
endfunction ()

macro (do_regex_success msg)
  string(REGEX MATCH "(0)(1)" output "01")
  check_for_success("${msg}")
endmacro ()

macro (do_regex_failure msg)
  string(REGEX MATCH "(0)(1)" output "12")
  check_for_failure("${msg}")
endmacro ()

do_regex_success("setting up initial state")

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(dummy) # Ensure cmMakefile::PushScope/PopScope work.

check_for_failure("checking after find_package")
do_regex_failure("clearing out results with a failing match")
do_regex_success("making a successful match before add_subdirectory")

add_subdirectory(subdir)

check_for_success("ensuring the subdirectory did not interfere with the parent") # Ensure that the subdir didn't mess with this scope.
