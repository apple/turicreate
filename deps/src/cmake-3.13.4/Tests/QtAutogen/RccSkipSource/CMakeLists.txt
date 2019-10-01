cmake_minimum_required(VERSION 3.10)
project(RccSkipSource)
include("../AutogenTest.cmake")

# Test for SKIP_AUTORCC and SKIP_AUTOGEN on an AUTORCC enabled target
set(skipRccSources
  skipRcc.cpp
  skipRccBad1.qrc
  skipRccBad2.qrc
  skipRccGood.qrc
)
set_property(SOURCE skipRccBad1.qrc PROPERTY SKIP_AUTORCC ON)
set_property(SOURCE skipRccBad2.qrc PROPERTY SKIP_AUTOGEN ON)
# AUTORCC enabled
add_executable(skipRccA ${skipRccSources})
set_property(TARGET skipRccA PROPERTY AUTORCC ON)
target_link_libraries(skipRccA ${QT_LIBRARIES})
# AUTORCC, AUTOUIC and AUTOMOC enabled
add_executable(skipRccB ${skipRccSources})
set_property(TARGET skipRccB PROPERTY AUTORCC ON)
set_property(TARGET skipRccB PROPERTY AUTOUIC ON)
set_property(TARGET skipRccB PROPERTY AUTOMOC ON)
target_link_libraries(skipRccB ${QT_LIBRARIES})
