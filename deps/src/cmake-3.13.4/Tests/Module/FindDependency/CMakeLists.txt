
cmake_minimum_required(VERSION 3.0)
project(FindDependency)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packages")

find_package(Pack1 REQUIRED)
find_package(Pack4 4.3 EXACT REQUIRED)

add_executable(FindDependency main.cpp)
target_link_libraries(FindDependency Pack1::Lib Pack4::Lib)
