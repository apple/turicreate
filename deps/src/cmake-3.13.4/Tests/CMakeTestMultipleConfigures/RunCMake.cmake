if(NOT DEFINED CMake_SOURCE_DIR)
  message(FATAL_ERROR "CMake_SOURCE_DIR not defined")
endif()

if(NOT DEFINED dir)
  message(FATAL_ERROR "dir not defined")
endif()

if(NOT DEFINED gen)
  message(FATAL_ERROR "gen not defined")
endif()

# Call cmake once to get a baseline/reference output build tree: "Build".
# Then call cmake N more times, each time making a copy of the entire
# build tree after cmake is done configuring/generating. At the end,
# analyze the diffs in the generated build trees. Expect no diffs.
#
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")

set(N 7)

# First setup source and binary trees:
#
execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory
  ${dir}/Source
)

execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory
  ${dir}/Build
)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMake_SOURCE_DIR}/Tests/CTestTest/SmallAndFast
  ${dir}/Source
)

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
  ${dir}/Build
)

# Patch SmallAndFast to build a .cxx executable too:
#
execute_process(COMMAND ${CMAKE_COMMAND} -E copy
  ${dir}/Source/echoargs.c
  ${dir}/Source/echoargs.cxx
)
file(APPEND "${dir}/Source/CMakeLists.txt" "\nadd_executable(echoargsCXX echoargs.cxx)\n")

# Loop N times, saving a copy of the configured/generated build tree each time:
#
foreach(i RANGE 1 ${N})
  # Equivalent sequence of shell commands:
  #
  message(STATUS "${i}: cd Build && cmake -G \"${gen}\" ../Source && cd .. && cp -r Build b${i}")

  # Run cmake:
  #
  execute_process(COMMAND ${CMAKE_COMMAND} -G ${gen} ../Source
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    WORKING_DIRECTORY ${dir}/Build
    )

  message(STATUS "result='${result}'")
  message(STATUS "stdout='${stdout}'")
  message(STATUS "stderr='${stderr}'")
  message(STATUS "")

  # Save this iteration of the Build directory:
  #
  execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory
    ${dir}/b${i}
    )
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${dir}/Build
    ${dir}/b${i}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    )

  message(STATUS "result='${result}'")
  message(STATUS "stdout='${stdout}'")
  message(STATUS "stderr='${stderr}'")
  message(STATUS "")
endforeach()


# Function to analyze diffs between two directories.
# Set DIFF_EXECUTABLE before calling if 'diff' is available.
#
function(analyze_directory_diffs d1 d2 diff_count_var)
  set(diffs 0)

  message(STATUS "Analyzing directory diffs between:")
  message(STATUS "  d1='${d1}'")
  message(STATUS "  d2='${d2}'")

  if(NOT "${d1}" STREQUAL "" AND NOT "${d2}" STREQUAL "")
    message(STATUS "info: analyzing directories")

    file(GLOB_RECURSE files1 RELATIVE "${d1}" "${d1}/*")
    file(GLOB_RECURSE files2 RELATIVE "${d2}" "${d2}/*")

    if("${files1}" STREQUAL "${files2}")
      message(STATUS "info: file lists the same")
      #message(STATUS "  files='${files1}'")

      foreach(f ${files1})
        execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
          ${d1}/${f}
          ${d2}/${f}
          RESULT_VARIABLE result
          OUTPUT_VARIABLE stdout
          ERROR_VARIABLE stderr
          )
        if(result STREQUAL 0)
          #message(STATUS "info: file '${f}' the same")
        else()
          math(EXPR diffs "${diffs} + 1")
          message(STATUS "warning: file '${f}' differs from d1 to d2")
          file(READ "${d1}/${f}" f1contents)
          message(STATUS "contents of file '${d1}/${f}'
[===[${f1contents}]===]")
          file(READ "${d2}/${f}" f2contents)
          message(STATUS "contents of file '${d2}/${f}'
[===[${f2contents}]===]")
          if(DIFF_EXECUTABLE)
            message(STATUS "diff of files '${d1}/${f}' '${d2}/${f}'")
            message(STATUS "[====[")
            execute_process(COMMAND ${DIFF_EXECUTABLE} "${d1}/${f}" "${d2}/${f}")
            message(STATUS "]====]")
          endif()
        endif()
      endforeach()
    else()
      math(EXPR diffs "${diffs} + 1")
      message(STATUS "warning: file *lists* differ - some files exist in d1/not-d2 or not-d1/d2...")
      message(STATUS "  files1='${files1}'")
      message(STATUS "  files2='${files2}'")
    endif()
  endif()

  set(${diff_count_var} ${diffs} PARENT_SCOPE)
endfunction()


# Analyze diffs between b1:b2, b2:b3, b3:b4, b4:b5 ... bN-1:bN.
# Expect no diffs.
#
find_program(DIFF_EXECUTABLE diff)
set(total_diffs 0)

foreach(i RANGE 2 ${N})
  math(EXPR prev "${i} - 1")
  set(count 0)
  analyze_directory_diffs(${dir}/b${prev} ${dir}/b${i} count)
  message(STATUS "diff count='${count}'")
  message(STATUS "")
  math(EXPR total_diffs "${total_diffs} + ${count}")
endforeach()

message(STATUS "CMAKE_COMMAND='${CMAKE_COMMAND}'")
message(STATUS "total_diffs='${total_diffs}'")
