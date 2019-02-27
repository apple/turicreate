set(failure_test_executables
  ${CMAKE_CURRENT_BINARY_DIR}/failure_test_targets)
file(WRITE ${failure_test_executables} "")

# Check if we should do anything. If the compiler doesn't support hidden
# visibility, the failure tests won't fail, so just write an empty targets
# list and punt.
if(NOT WIN32 AND NOT CYGWIN AND NOT COMPILER_HAS_HIDDEN_VISIBILITY)
  return()
endif()

# Read the input source file
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/exportheader_test.cpp content_post)
set(content_pre "")

# Generate source files for failure test executables
set(counter 0)
while(1)
  # Find first occurrence of link error marker in remaining content
  string(REGEX MATCH "//([^;\n]+;) LINK ERROR( [(][^)]+[)])?\n(.*)"
    match "${content_post}")
  if(match STREQUAL "")
    # No more matches
    break()
  endif()

  # Shift content buffers and extract failing statement
  string(LENGTH "${content_post}" content_post_length)
  string(LENGTH "${match}" matched_length)
  math(EXPR shift_length "${content_post_length} - ${matched_length}")

  string(SUBSTRING "${content_post}" 0 ${shift_length} shift)
  set(content_pre "${content_pre}${shift}")
  set(content_post "${CMAKE_MATCH_3}")
  set(content_active "//${CMAKE_MATCH_1} LINK ERROR${CMAKE_MATCH_2}")
  set(statement "${CMAKE_MATCH_1}")

  # Check if potential error is conditional, and evaluate condition if so
  string(REGEX REPLACE " [(]([^)]+)[)]" "\\1" condition "${CMAKE_MATCH_2}")
  if(NOT condition STREQUAL "")
    string(REGEX REPLACE " +" ";" condition "${condition}")
    if(${condition})
    else()
      message(STATUS "Not testing '${statement}'; "
                     "condition (${condition}) is FALSE")
      set(content_pre "${content_pre}// link error removed\n")
      continue()
    endif()
  endif()

  if(NOT skip)
    message(STATUS "Creating failure test for '${statement}'")
    math(EXPR counter "${counter} + 1")

    # Write new source file
    set(out ${CMAKE_CURRENT_BINARY_DIR}/exportheader_failtest-${counter}.cpp)
    file(WRITE ${out} "${content_pre}${statement}\n${content_post}")

    # Add executable for failure test
    add_executable(GEH-fail-${counter} EXCLUDE_FROM_ALL ${out})
    target_link_libraries(GEH-fail-${counter} ${link_libraries})
    file(APPEND ${failure_test_executables} "GEH-fail-${counter}\n")
  endif()

  # Add placeholder where failing statement was removed
  set(content_pre "${content_pre}${content_active}\n")
endwhile()
