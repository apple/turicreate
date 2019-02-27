cmake_minimum_required(VERSION 3.4)
include(RunCMake)

# TODO Migrate Tests/BundleUtilities here

run_cmake(CMP0080-OLD)
run_cmake(CMP0080-NEW)
run_cmake(CMP0080-WARN)
run_cmake_command(CMP0080-COMMAND-OLD ${CMAKE_COMMAND} -DCMP0080_VALUE:STRING=OLD -P ${RunCMake_SOURCE_DIR}/CMP0080-COMMAND.cmake)
run_cmake_command(CMP0080-COMMAND-NEW ${CMAKE_COMMAND} -DCMP0080_VALUE:STRING=NEW -P ${RunCMake_SOURCE_DIR}/CMP0080-COMMAND.cmake)
run_cmake_command(CMP0080-COMMAND-WARN ${CMAKE_COMMAND} -P ${RunCMake_SOURCE_DIR}/CMP0080-COMMAND.cmake)
