
cmake_minimum_required(VERSION 3.0)

project(ConfigSources)

add_library(iface INTERFACE)
set_property(TARGET iface PROPERTY INTERFACE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/iface_src.cpp"
  "$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/iface_debug_src.cpp>"
  "$<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/does_not_exist.cpp>"
)

add_executable(ConfigSources
  $<$<CONFIG:Debug>:main.cpp>
  $<$<CONFIG:Release>:does_not_exist.cpp>
)
target_link_libraries(ConfigSources iface)
