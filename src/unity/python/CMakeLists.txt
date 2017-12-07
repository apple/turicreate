project(unity)

add_subdirectory(turicreate)


# With CMake, a clean separation can be made between the source tree and the
# build tree.  When all source is compiled, as with pure C/C++, the source is
# no-longer needed in the build tree.  However, with pure *.py source, the
# source is processed directly.  To handle this, we reproduce the availability
# of the source files in the build tree.

# Copy all the open source python stuff over first

# Overwrite with the proprietary stuff
ADD_CUSTOM_TARGET (python_source ALL)
ADD_CUSTOM_COMMAND(
  COMMENT "build prop python source"
  COMMAND
  ${CMAKE_COMMAND} -E
  copy_directory ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}

  TARGET python_source
)
add_dependencies(python_source numpy_integration)


ADD_CUSTOM_TARGET (clean_python)
add_dependencies(clean_python clean_turicreate_binaries clean_cython)

ADD_CUSTOM_COMMAND(
  COMMENT "make clean python directory"
  COMMAND
  ${CMAKE_COMMAND} -P
  ${CMAKE_SOURCE_DIR}/cmake/RemovePythonSourceTree.cmake
  ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  TARGET clean_python
)
