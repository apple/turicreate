
cmake_minimum_required(VERSION 3.12)

project(target_link_directories LANGUAGES C)

add_library(target_link_directories SHARED LinkDirectoriesLib.c)
# Test no items
target_link_directories(target_link_directories PRIVATE)

add_library(target_link_directories_2 SHARED EXCLUDE_FROM_ALL LinkDirectoriesLib.c)
target_link_directories(target_link_directories_2 PRIVATE /private/dir INTERFACE /interface/dir)
get_target_property(result target_link_directories_2 LINK_DIRECTORIES)
if (NOT result MATCHES "/private/dir")
  message(SEND_ERROR "${result} target_link_directories not populated the LINK_DIRECTORIES target property")
endif()
get_target_property(result target_link_directories_2 INTERFACE_LINK_DIRECTORIES)
if (NOT result MATCHES "/interface/dir")
  message(SEND_ERROR "target_link_directories not populated the INTERFACE_LINK_DIRECTORIES target property of shared library")
endif()

add_library(target_link_directories_3 STATIC EXCLUDE_FROM_ALL LinkDirectoriesLib.c)
target_link_directories(target_link_directories_3 INTERFACE /interface/dir)
get_target_property(result target_link_directories_3 INTERFACE_LINK_DIRECTORIES)
if (NOT result MATCHES "/interface/dir")
  message(SEND_ERROR "target_link_directories not populated the INTERFACE_LINK_DIRECTORIES target property of static library")
endif()

add_library(target_link_directories_4 SHARED EXCLUDE_FROM_ALL LinkDirectoriesLib.c)
target_link_directories(target_link_directories_4 PRIVATE relative/dir)
get_target_property(result target_link_directories_4 LINK_DIRECTORIES)
if (NOT result MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/relative/dir")
  message(SEND_ERROR "target_link_directories not populated the LINK_DIRECTORIES with relative path")
endif()

add_subdirectory(subdir)
target_link_directories(target_link_directories_5 PRIVATE relative/dir)
get_target_property(result target_link_directories_5 LINK_DIRECTORIES)
if (NOT result MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/relative/dir")
  message(SEND_ERROR "target_link_directories not populated the LINK_DIRECTORIES with relative path")
endif()
