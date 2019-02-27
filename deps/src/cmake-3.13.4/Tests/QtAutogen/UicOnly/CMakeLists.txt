cmake_minimum_required(VERSION 3.10)
project(UicOnly)
include("../AutogenTest.cmake")

# Test AUTOUIC being enabled only
add_executable(uicOnly main.cpp UicOnly.cpp)
set_property(TARGET uicOnly PROPERTY AUTOUIC ON)
target_link_libraries(uicOnly ${QT_LIBRARIES})
