cmake_minimum_required(VERSION 3.10)
project(RccOffMocLibrary)
include("../AutogenTest.cmake")

# Add not_generated_file.qrc to the source list to get the file-level
# dependency, but don't generate a c++ file from it.  Disable the AUTORCC
# feature for this target.  This tests that qrc files in the sources don't
# have an effect on generation if AUTORCC is off.
add_library(empty STATIC empty.cpp not_generated_file.qrc)
set_target_properties(empty PROPERTIES AUTORCC OFF)
set_target_properties(empty PROPERTIES AUTOMOC TRUE)
target_link_libraries(empty no_link_language)
add_library(no_link_language STATIC empty.h)
set_target_properties(no_link_language PROPERTIES AUTOMOC TRUE)
# Pass Qt compiler features to targets that don't link against Qt
target_compile_features(no_link_language PRIVATE ${QT_COMPILE_FEATURES})
target_compile_features(empty PRIVATE ${QT_COMPILE_FEATURES})
