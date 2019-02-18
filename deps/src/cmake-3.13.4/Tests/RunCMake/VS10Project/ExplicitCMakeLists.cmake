set(CMAKE_CONFIGURATION_TYPES Debug)
enable_language(CXX)
add_executable(foo foo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
