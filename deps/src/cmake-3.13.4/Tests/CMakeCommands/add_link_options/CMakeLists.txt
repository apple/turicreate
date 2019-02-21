cmake_minimum_required(VERSION 3.11)

project(add_link_options LANGUAGES C)


add_link_options(-LINK_FLAG)

add_executable(add_link_options EXCLUDE_FROM_ALL LinkOptionsExe.c)

get_target_property(result add_link_options LINK_OPTIONS)
if (NOT result MATCHES "-LINK_FLAG")
  message(SEND_ERROR "add_link_options not populated the LINK_OPTIONS target property")
endif()


add_library(imp UNKNOWN IMPORTED)
get_target_property(result imp LINK_OPTIONS)
if (result)
  message(FATAL_ERROR "add_link_options populated the LINK_OPTIONS target property")
endif()
