cmake_minimum_required(VERSION 3.10)
project(RccOnly)
include("../AutogenTest.cmake")

# Test AUTORCC being enabled only
add_executable(rccOnly rccOnly.cpp rccOnlyRes.qrc)
set_property(TARGET rccOnly PROPERTY AUTORCC ON)
target_link_libraries(rccOnly ${QT_QTCORE_TARGET})
