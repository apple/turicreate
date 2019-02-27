cmake_minimum_required(VERSION 3.10)
project(UicSkipSource)
include("../AutogenTest.cmake")

# Test for SKIP_AUTOUIC and SKIP_AUTOGEN on an AUTOUIC enabled target
set(skipUicSources
  skipUic.cpp
  skipUicGen.cpp
  skipUicNoGen1.cpp
  skipUicNoGen2.cpp
)
set_property(SOURCE skipUicNoGen1.cpp PROPERTY SKIP_AUTOUIC ON)
set_property(SOURCE skipUicNoGen2.cpp PROPERTY SKIP_AUTOGEN ON)
# AUTOUIC enabled
add_executable(skipUicA ${skipUicSources})
set_property(TARGET skipUicA PROPERTY AUTOUIC ON)
target_link_libraries(skipUicA ${QT_LIBRARIES})
# AUTOUIC and AUTOMOC enabled
add_executable(skipUicB ${skipUicSources})
set_property(TARGET skipUicB PROPERTY AUTOUIC ON)
set_property(TARGET skipUicB PROPERTY AUTOMOC ON)
target_link_libraries(skipUicB ${QT_LIBRARIES})
