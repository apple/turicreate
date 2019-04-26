
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

add_library(libB SHARED libB.cpp)
generate_export_header(libB)

target_link_libraries(libB LINK_PUBLIC libA)
