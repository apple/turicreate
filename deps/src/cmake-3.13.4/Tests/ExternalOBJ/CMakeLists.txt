cmake_minimum_required (VERSION 2.6)
project (ExternalOBJ)

if(APPLE)
  # set _CMAKE_OSX_MACHINE to umame -m
  exec_program(uname ARGS -m OUTPUT_VARIABLE _CMAKE_OSX_MACHINE)
  # check for Power PC and change to ppc
  if("${_CMAKE_OSX_MACHINE}" MATCHES "Power")
    set(_CMAKE_OSX_MACHINE ppc)
  endif()
  set(CMAKE_OSX_ARCHITECTURES ${_CMAKE_OSX_MACHINE})
endif()

# Build the external object file.
try_compile(EXTERNAL_OBJECT_BUILT
  ${ExternalOBJ_BINARY_DIR}/Object
  ${ExternalOBJ_SOURCE_DIR}/Object
  Object
  external
  OUTPUT_VARIABLE OUTPUT
  )
if(EXTERNAL_OBJECT_BUILT)
  message(
    "Building external_object.cxx succeeded with the following output:\n"
    "[${OUTPUT}]"
    )
else()
  message(FATAL_ERROR
    "Building external_object.cxx failed with the following output:\n"
    "[${OUTPUT}]"
    )
endif()

# Find the external object file.
set(DIR ${ExternalOBJ_BINARY_DIR}/Object)
file(GLOB_RECURSE EXTERNAL_OBJECT
  "${DIR}/external_object*${CMAKE_CXX_OUTPUT_EXTENSION}")
if(EXTERNAL_OBJECT)
  list (GET EXTERNAL_OBJECT 0 EXTERNAL_OBJECT)
  message("Found \"${EXTERNAL_OBJECT}\".")
else()
  message(FATAL_ERROR "Could not find external object.")
endif()

# Test creation of external objects by custom commands.
set(CUSTOM_OBJECT
  ${CMAKE_CURRENT_BINARY_DIR}/custom_object${CMAKE_C_OUTPUT_EXTENSION})
add_custom_command(
  OUTPUT ${CUSTOM_OBJECT}
  COMMAND ${CMAKE_COMMAND} -E copy ${EXTERNAL_OBJECT} ${CUSTOM_OBJECT}
  DEPENDS ${EXTERNAL_OBJECT}
  )

message("${EXTERNAL_OBJECT}")
# Build an executable using the external object file.
add_executable(ExternalOBJ executable.cxx ${CUSTOM_OBJECT})
# A bug showed up in VS2010 where an object file that was
# part of a custom command output worked, but ones that were
# not didn't work.  So, repeat the executable using the object
# directly and not from the output of the copy.
add_executable(ExternalOBJ2 executable.cxx ${EXTERNAL_OBJECT})

add_subdirectory(Sub)
