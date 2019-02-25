cmake_minimum_required(VERSION 3.8)
project(mocPlugin CXX)

set(CMAKE_AUTOMOC_DEPEND_FILTERS
  "A_CUSTOM_MACRO"
  "[\n][ \t]*A_CUSTOM_MACRO[ \t\r\n]*\\([^,]+,[ \t\r\n]*\"([^\"]+)\""
  )

if (NOT QT_TEST_VERSION STREQUAL 5)
  message(SEND_ERROR "Invalid Qt version specified.")
endif()
find_package(Qt5Widgets REQUIRED)

if(Qt5_POSITION_INDEPENDENT_CODE AND CMAKE_CXX_COMPILE_OPTIONS_PIC)
  add_definitions(${CMAKE_CXX_COMPILE_OPTIONS_PIC})
endif()

configure_file(jsonIn/StyleC.json jsonFiles/StyleC.json)
configure_file(jsonIn/StyleD.json jsonFiles/sub/StyleD.json)
configure_file(jsonIn/StyleC.json jsonFiles/StyleC_Custom.json)
configure_file(jsonIn/StyleD.json jsonFiles/sub/StyleD_Custom.json)

# Enable automoc
set(CMAKE_AUTOMOC TRUE)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/jsonFiles")
link_libraries(Qt5::Widgets)

add_library(PlugA STATIC StyleA.cpp)
add_library(PlugB STATIC StyleB.cpp)
add_library(PlugC STATIC StyleC.cpp)
add_library(PlugD STATIC StyleD.cpp)
add_library(PlugE STATIC StyleE.cpp)
