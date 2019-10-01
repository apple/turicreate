enable_language(C)
# Look for a source tree left by enable_language internal checks.
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/CMakeLists.txt)
  message(FATAL_ERROR "--debug-trycompile should leave the source behind")
endif()
