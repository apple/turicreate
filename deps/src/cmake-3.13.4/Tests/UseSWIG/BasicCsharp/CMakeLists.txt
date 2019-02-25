cmake_minimum_required(VERSION 3.12...3.13)

project(TestBasicCsharp CXX CSharp)

include(CTest)

set(language "csharp")

include (../BasicConfiguration.cmake)

set_source_files_properties(Square.cs Circle.cs Shape.cs PROPERTIES GENERATED 1)
add_library(example_cs SHARED  $<TARGET_PROPERTY:example,SWIG_SUPPORT_FILES> Square.cs Circle.cs Shape.cs)
set_target_properties(example_cs PROPERTIES LINKER_LANGUAGE CSharp)
target_link_libraries(example_cs example)

add_executable(runme ${CMAKE_CURRENT_SOURCE_DIR}/../runme.cs)
target_link_libraries(runme example_cs)
set_target_properties(runme PROPERTIES LINKER_LANGUAGE CSharp)

add_test (NAME BasicCsharp
  COMMAND $<TARGET_FILE:runme>)
