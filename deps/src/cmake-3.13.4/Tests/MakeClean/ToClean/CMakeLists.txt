cmake_minimum_required(VERSION 2.6)
project(ToClean)

# Build a simple project.
add_executable(toclean toclean.cxx)

# List some build-time-generated files.
set(TOCLEAN_FILES ${TOCLEAN_FILES}
  "${ToClean_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/toclean.dir/toclean.cxx${CMAKE_CXX_OUTPUT_EXTENSION}")

# Create a file that must be registered for cleaning.
file(WRITE "${ToClean_BINARY_DIR}/Registered.txt"
  "File registered for cleaning.\n")
set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${ToClean_BINARY_DIR}/Registered.txt")
set(TOCLEAN_FILES ${TOCLEAN_FILES} "${ToClean_BINARY_DIR}/Registered.txt")

# Create a custom command whose output should be cleaned.
add_custom_command(OUTPUT ${ToClean_BINARY_DIR}/generated.txt
  DEPENDS ${ToClean_SOURCE_DIR}/toclean.cxx
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy ${ToClean_SOURCE_DIR}/toclean.cxx
               ${ToClean_BINARY_DIR}/generated.txt
  )
add_custom_target(generate ALL DEPENDS ${ToClean_BINARY_DIR}/generated.txt)
set(TOCLEAN_FILES ${TOCLEAN_FILES} "${ToClean_BINARY_DIR}/generated.txt")

# Create a custom command whose output should be cleaned, but whose name
# is not known until generate-time
set(copied_exe "$<TARGET_FILE_DIR:toclean>/toclean_copy${CMAKE_EXECUTABLE_SUFFIX}")
add_custom_command(TARGET toclean POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy $<TARGET_FILE:toclean>
               ${copied_exe}
  )
set_property(DIRECTORY APPEND PROPERTY
  ADDITIONAL_MAKE_CLEAN_FILES ${copied_exe})
list(APPEND TOCLEAN_FILES "${ToClean_BINARY_DIR}/toclean_copy${CMAKE_EXECUTABLE_SUFFIX}")

# Configure a file listing these build-time-generated files.
configure_file(${ToClean_SOURCE_DIR}/ToCleanFiles.cmake.in
               ${ToClean_BINARY_DIR}/ToCleanFiles.cmake @ONLY)
