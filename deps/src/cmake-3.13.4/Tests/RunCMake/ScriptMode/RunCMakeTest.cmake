include(RunCMake)

run_cmake_command(set_directory_properties ${CMAKE_COMMAND} -P ${RunCMake_SOURCE_DIR}/set_directory_properties.cmake)
