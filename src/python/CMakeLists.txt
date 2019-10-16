project(unity)

add_subdirectory(turicreate)


# With CMake, a clean separation can be made between the source tree and the
# build tree.  When all source is compiled, as with pure C/C++, the source is
# no-longer needed in the build tree.  However, with pure *.py source, the
# source is processed directly.  To handle this, we reproduce the availability
# of the source files in the build tree.

# Copy all the open source python stuff over first

# Overwrite with the proprietary stuff

make_copy_target(python_source
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in
    ${CMAKE_CURRENT_SOURCE_DIR}/README.rst
    ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/setup.cfg
    ${CMAKE_CURRENT_SOURCE_DIR}/setup.py
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/turicreate
)
add_dependencies(python_source numpy_integration tctensorflow)

add_custom_target (clean_python)
add_dependencies(clean_python clean_turicreate_binaries clean_cython)

add_custom_command(
  COMMENT "make clean python directory"
  COMMAND
  ${CMAKE_COMMAND} -P
  ${CMAKE_SOURCE_DIR}/cmake/RemovePythonSourceTree.cmake
  ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  TARGET clean_python
)
