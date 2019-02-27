
cmake_minimum_required(VERSION 2.8.12.20131125 FATAL_ERROR)

project(genex_link)

set(_main_cpp ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)
file(WRITE ${_main_cpp}
  "int main(int argc, char** argv) { return 0; }\n"
)

add_library(foo::bar INTERFACE IMPORTED)
set_target_properties(foo::bar
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
    # When not using a generator expression here, no error is generated
    INTERFACE_LINK_LIBRARIES "$<$<NOT:$<CONFIG:DEBUG>>:foo_bar.lib>"
)

add_executable(main ${_main_cpp})
target_include_directories(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(main foo::bar)
