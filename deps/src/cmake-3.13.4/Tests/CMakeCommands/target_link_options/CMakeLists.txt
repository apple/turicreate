
cmake_minimum_required(VERSION 3.11)

project(target_link_options LANGUAGES C)

add_library(target_link_options SHARED LinkOptionsLib.c)
# Test no items
target_link_options(target_link_options PRIVATE)

add_library(target_link_options_2 SHARED EXCLUDE_FROM_ALL LinkOptionsLib.c)
target_link_options(target_link_options_2 PRIVATE -PRIVATE_FLAG INTERFACE -INTERFACE_FLAG)
get_target_property(result target_link_options_2 LINK_OPTIONS)
if (NOT result MATCHES "-PRIVATE_FLAG")
  message(SEND_ERROR "target_link_options not populated the LINK_OPTIONS target property")
endif()
get_target_property(result target_link_options_2 INTERFACE_LINK_OPTIONS)
if (NOT result MATCHES "-INTERFACE_FLAG")
  message(SEND_ERROR "target_link_options not populated the INTERFACE_LINK_OPTIONS target property of shared library")
endif()

add_library(target_link_options_3 STATIC EXCLUDE_FROM_ALL LinkOptionsLib.c)
target_link_options(target_link_options_3 INTERFACE -INTERFACE_FLAG)
get_target_property(result target_link_options_3 INTERFACE_LINK_OPTIONS)
if (NOT result MATCHES "-INTERFACE_FLAG")
  message(SEND_ERROR "target_link_options not populated the INTERFACE_LINK_OPTIONS target property of static library")
endif()

add_library(target_link_options_4 SHARED EXCLUDE_FROM_ALL LinkOptionsLib.c)
target_link_options(target_link_options_4 PRIVATE -PRIVATE_FLAG)
target_link_options(target_link_options_4 BEFORE PRIVATE -BEFORE_PRIVATE_FLAG)
get_target_property(result target_link_options_4 LINK_OPTIONS)
if (NOT result MATCHES "-BEFORE_PRIVATE_FLAG.*-PRIVATE_FLAG")
  message(SEND_ERROR "target_link_options not managing correctly 'BEFORE' keyword")
endif()
