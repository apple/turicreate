cmake_minimum_required (VERSION 2.6)
project(MakeClean)

# Build the to-clean project.
try_compile(TOCLEAN_BUILT
  ${MakeClean_BINARY_DIR}/ToClean
  ${MakeClean_SOURCE_DIR}/ToClean
  ToClean
  OUTPUT_VARIABLE OUTPUT
  )
if(TOCLEAN_BUILT)
  message(
    "Building ToClean succeeded with the following output:\n"
    "[${OUTPUT}]"
    )
else()
  message(FATAL_ERROR
    "Building ToClean failed with the following output:\n"
    "[${OUTPUT}]"
    )
endif()

# Get the set of files to check from the ToClean project.
include(${MakeClean_BINARY_DIR}/ToClean/ToCleanFiles.cmake)

# Check for the existence of the files.
foreach(f ${TOCLEAN_FILES})
  if(EXISTS "${f}")
  else()
    message(FATAL_ERROR "File \"${f}\" does not exist!")
  endif()
endforeach()

# Configure an executable to check that all the files are missing.
set(CHECK_FILES)
foreach(f ${TOCLEAN_FILES})
  string(APPEND CHECK_FILES "      \"${f}\",\n")
endforeach()
configure_file(${MakeClean_SOURCE_DIR}/check_clean.c.in
               ${MakeClean_BINARY_DIR}/check_clean.c @ONLY)
add_executable(check_clean ${MakeClean_BINARY_DIR}/check_clean.c)

# After the executable builds, clean the files.
add_custom_command(
  TARGET check_clean
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} --build ${MakeClean_BINARY_DIR}/ToClean
          --target clean
  COMMENT "Clean the ToClean Project"
  )
