
cmake_minimum_required(VERSION 3.0)

project(SourcesProperty)

add_library(iface INTERFACE)
set_property(TARGET iface PROPERTY INTERFACE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/iface.cpp"
)

add_executable(SourcesProperty main.cpp)
target_link_libraries(SourcesProperty iface)

set_property(TARGET SourcesProperty APPEND PROPERTY SOURCES prop.cpp)
