cmake_minimum_required(VERSION 3.10)
project(RccEmpty)
include("../AutogenTest.cmake")

# Test AUTORCC on a .qrc file with no resource files
add_executable(rccEmpty rccEmpty.cpp rccEmptyRes.qrc)
set_property(TARGET rccEmpty PROPERTY AUTORCC ON)
target_link_libraries(rccEmpty ${QT_QTCORE_TARGET})
