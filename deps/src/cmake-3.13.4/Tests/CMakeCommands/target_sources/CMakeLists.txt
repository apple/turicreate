
cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0076 NEW)

project(target_sources)

add_library(target_sources_lib)
target_compile_definitions(target_sources_lib PRIVATE "-DIS_LIB")
add_subdirectory(subdir)

set(subdir_fullpath "${CMAKE_CURRENT_LIST_DIR}/subdir")

get_property(target_sources_lib_property TARGET target_sources_lib PROPERTY SOURCES)
if (NOT "$<1:${subdir_fullpath}/subdir_empty_1.cpp>" IN_LIST target_sources_lib_property)
  message(SEND_ERROR "target_sources_lib: Generator expression to absolute sub directory file not found")
endif()
if (NOT "$<1:${subdir_fullpath}/../empty_1.cpp>" IN_LIST target_sources_lib_property)
  message(SEND_ERROR "target_sources_lib: Generator expression to absolute main directory file not found")
endif()
if (NOT "${subdir_fullpath}/subdir_empty_2.cpp" IN_LIST target_sources_lib_property)
  message(SEND_ERROR "target_sources_lib: Relative sub directory file not converted to absolute")
endif()
if (NOT "$<1:empty_2.cpp>" IN_LIST target_sources_lib_property)
  message(SEND_ERROR "target_sources_lib: Generator expression to relative main directory file not found")
endif()
if (NOT "${subdir_fullpath}/../empty_3.cpp" IN_LIST target_sources_lib_property)
  message(SEND_ERROR "target_sources_lib: Relative main directory file not converted to absolute")
endif()

add_executable(target_sources main.cpp)
target_link_libraries(target_sources target_sources_lib)

get_property(target_sources_property TARGET target_sources PROPERTY SOURCES)
if (NOT "main.cpp" IN_LIST target_sources_property)
  message(SEND_ERROR "target_sources: Relative main directory file converted to absolute")
endif()
