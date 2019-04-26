set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

include(CMakeFindDependencyMacro)

find_dependency(Pack1 1.1 EXACT REQUIRED)
