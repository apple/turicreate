cmake_minimum_required(VERSION 3.10)
project(MocIncludeRelaxed)
include("../AutogenTest.cmake")

# Test moc include patterns
set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)

# Shared executable
set(MOC_INCLUDE_NAME "mocIncludeRelaxed")
include(${CMAKE_CURRENT_SOURCE_DIR}/../MocInclude/shared.cmake)

# Relaxed only executable
add_executable(mocIncludeRelaxedOnly
  RObjA.cpp
  RObjB.cpp
  RObjC.cpp
  RMain.cpp
)
target_link_libraries(mocIncludeRelaxedOnly ${QT_LIBRARIES})
set_target_properties(mocIncludeRelaxedOnly PROPERTIES AUTOMOC ON)
