cmake_minimum_required(VERSION 3.10)
project(MocPlugin)
include("../../AutogenTest.cmake")

if (QT_TEST_VERSION LESS 5)
  message(SEND_ERROR "Qt 5 or higher required.")
endif()

set(CMAKE_AUTOMOC_DEPEND_FILTERS
  "A_CUSTOM_MACRO"
  "[\n][ \t]*A_CUSTOM_MACRO[ \t\r\n]*\\([^,]+,[ \t\r\n]*\"([^\"]+)\""
)

configure_file(jsonIn/StyleC.json jsonFiles/StyleC.json)
configure_file(jsonIn/StyleC.json jsonFiles/StyleC_Custom.json)
configure_file(jsonIn/StyleD.json jsonFiles/sub/StyleD.json)
configure_file(jsonIn/StyleD.json jsonFiles/sub/StyleD_Custom.json)
configure_file(jsonIn/StyleE.json jsonFiles/StyleE.json)
configure_file(jsonIn/StyleE.json jsonFiles/StyleE_Custom.json)

# Enable AUTOMOC
set(CMAKE_AUTOMOC TRUE)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/jsonFiles")
link_libraries(Qt5::Widgets)

add_library(PlugA STATIC StyleA.cpp)
add_library(PlugB STATIC StyleB.cpp)
add_library(PlugC STATIC StyleC.cpp)
add_library(PlugD STATIC StyleD.cpp)
add_library(PlugE STATIC StyleE.cpp)
