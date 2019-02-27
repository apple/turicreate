cmake_minimum_required(VERSION 3.10)
project(MocIncludeStrict)
include("../AutogenTest.cmake")

# Test moc include patterns
set(CMAKE_AUTOMOC_RELAXED_MODE FALSE)

# Shared executable
set(MOC_INCLUDE_NAME "mocIncludeStrict")
include(${CMAKE_CURRENT_SOURCE_DIR}/../MocInclude/shared.cmake)
