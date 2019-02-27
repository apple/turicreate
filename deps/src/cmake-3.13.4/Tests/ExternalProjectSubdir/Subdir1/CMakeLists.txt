cmake_minimum_required(VERSION 3.2)
project(Subdir1 NONE)

if(NOT "${NORMAL_VAR}" STREQUAL "NORMAL_VALUE")
  message(SEND_ERROR "NORMAL_VAR != 'NORMAL_VALUE'")
endif()

if(NOT "${GENEX_VAR}" STREQUAL "GENEX_VALUE")
  message(SEND_ERROR "GENEX_VAR != 'GENEX_VALUE'")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../PreSubdir1.txt")
  message(SEND_ERROR "../PreSubdir1.txt not provided!")
endif()
