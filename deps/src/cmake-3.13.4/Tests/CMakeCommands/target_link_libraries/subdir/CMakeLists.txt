
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_library(subdirlib SHARED subdirlib.cpp)
generate_export_header(subdirlib)
