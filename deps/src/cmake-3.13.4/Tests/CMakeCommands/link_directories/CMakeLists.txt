cmake_minimum_required(VERSION 3.12)

project(link_directories LANGUAGES C)


link_directories(/A)
link_directories(BEFORE /B)

set(CMAKE_LINK_DIRECTORIES_BEFORE ON)
link_directories(/C)

get_directory_property(result LINK_DIRECTORIES)
if (NOT result MATCHES "/C;/B;/A")
  message(SEND_ERROR "link_directories not populated the LINK_DIRECTORIES directory property")
endif()


add_executable(link_directories EXCLUDE_FROM_ALL LinkDirectoriesExe.c)

get_target_property(result link_directories LINK_DIRECTORIES)
if (NOT result MATCHES "/C;/B;/A")
  message(SEND_ERROR "link_directories not populated the LINK_DIRECTORIES target property")
endif()


add_library(imp UNKNOWN IMPORTED)
get_target_property(result imp LINK_DIRECTORIES)
if (result)
  message(FATAL_ERROR "link_directories populated the LINK_DIRECTORIES target property")
endif()
