message(STATUS "Checking for curses support")

# Try compiling a simple project using curses.
# Pass in any cache entries that the user may have set.
set(CMakeCheckCurses_ARGS "")
foreach(v
    CURSES_INCLUDE_PATH
    CURSES_CURSES_LIBRARY
    CURSES_NCURSES_LIBRARY
    CURSES_EXTRA_LIBRARY
    CURSES_FORM_LIBRARY
    )
  if(${v})
    list(APPEND CMakeCheckCurses_ARGS -D${v}=${${v}})
  endif()
endforeach()
file(REMOVE_RECURSE "${CMake_BINARY_DIR}/Source/Checks/Curses-build")
try_compile(CMakeCheckCurses_COMPILED
  ${CMake_BINARY_DIR}/Source/Checks/Curses-build
  ${CMake_SOURCE_DIR}/Source/Checks/Curses
  CheckCurses # project name
  CheckCurses # target name
  CMAKE_FLAGS
    "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
    ${CMakeCheckCurses_ARGS}
  OUTPUT_VARIABLE CMakeCheckCurses_OUTPUT
  )

# Convert result from cache entry to normal variable.
set(CMakeCheckCurses_COMPILED "${CMakeCheckCurses_COMPILED}")
unset(CMakeCheckCurses_COMPILED CACHE)

if(CMakeCheckCurses_COMPILED)
  message(STATUS "Checking for curses support - Success")
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
    "Checking for curses support passed with the following output:\n${CMakeCheckCurses_OUTPUT}\n\n")
else()
  message(STATUS "Checking for curses support - Failed")
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Checking for curses support failed with the following output:\n${CMakeCheckCurses_OUTPUT}\n\n")
endif()
