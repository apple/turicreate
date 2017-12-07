
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(subdirlib SHARED subdir.cpp)
generate_export_header(subdirlib)
