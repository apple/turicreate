cmake_minimum_required(VERSION 3.8)
project(TestFindDoxygenDot VERSION 1.0 LANGUAGES NONE)

# Testing a new signature w/ components
find_package(Doxygen REQUIRED COMPONENTS dot)

# No backwards compatibility expected when component form is used
if(TARGET Doxygen::doxygen)
    if(DOXYGEN)
        message(FATAL_ERROR "DOXYGEN unexpectedly defined with component form")
    endif()
else()
    message(FATAL_ERROR "Import target Doxygen::doxygen not defined")
endif()

if(NOT TARGET Doxygen::dot)
    message(FATAL_ERROR "Import target Doxygen::dot not defined")
endif()
