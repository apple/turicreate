foreach (_retval 0 1)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/memtester.cxx.in" "${CMAKE_CURRENT_BINARY_DIR}/ret${_retval}.cxx" @ONLY)
endforeach ()

include_directories(${CMake_SOURCE_DIR}/Source ${CMake_BINARY_DIR}/Source)

# create binaries that we will use as a pseudo memory checker
add_executable(pseudo_valgrind "${CMAKE_CURRENT_BINARY_DIR}/ret0.cxx")
set_target_properties(pseudo_valgrind PROPERTIES OUTPUT_NAME valgrind)
target_link_libraries(pseudo_valgrind CMakeLib)

add_executable(pseudo_purify "${CMAKE_CURRENT_BINARY_DIR}/ret0.cxx")
set_target_properties(pseudo_purify PROPERTIES OUTPUT_NAME purify)
target_link_libraries(pseudo_purify CMakeLib)
add_executable(pseudo_BC "${CMAKE_CURRENT_BINARY_DIR}/ret0.cxx")
set_target_properties(pseudo_BC PROPERTIES OUTPUT_NAME BC)
target_link_libraries(pseudo_BC CMakeLib)

# binary to be used as pre- and post-memcheck command that fails
add_executable(memcheck_fail "${CMAKE_CURRENT_BINARY_DIR}/ret1.cxx")
target_link_libraries(memcheck_fail CMakeLib)

# Binaries that are used as memchecker that do not write the expected
# output file. Need to be in their own subdirectory as they have the
# same filenames.
add_subdirectory(NoLog)
