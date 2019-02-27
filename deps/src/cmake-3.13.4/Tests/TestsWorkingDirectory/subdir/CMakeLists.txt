add_test(NAME WorkingDirectory-Subdir0 COMMAND WorkingDirectory "${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME WorkingDirectory-Subdir1 COMMAND WorkingDirectory "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(WorkingDirectory-Subdir1 PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

string(REGEX REPLACE "/[^/]*$" "" _parent_dir "${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME WorkingDirectory-Subdir2 COMMAND WorkingDirectory "${_parent_dir}")
set_tests_properties(WorkingDirectory-Subdir2 PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
)

set(_default_cwd "${CMAKE_CURRENT_BINARY_DIR}")

# FIXME: How to deal with /debug, /release, etc. with VS or Xcode?
if(${CMAKE_GENERATOR} MATCHES "Makefiles")
add_test(WorkingDirectory-Subdir3 ${EXECUTABLE_OUTPUT_PATH}/WorkingDirectory ${_default_cwd})
endif()

add_test(NAME WorkingDirectory-Subdir4 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND WorkingDirectory ${CMAKE_CURRENT_BINARY_DIR})

string(REGEX REPLACE "/[^/]*$" "" _parent_dir "${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME WorkingDirectory-Subdir5 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/.. COMMAND WorkingDirectory ${_parent_dir})

# FIXME: How to deal with /debug, /release, etc. with VS or Xcode?
if(${CMAKE_GENERATOR} MATCHES "Makefiles")
add_test(WorkingDirectory-Subdir6 ${EXECUTABLE_OUTPUT_PATH}/WorkingDirectory ${_default_cwd} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
endif()
