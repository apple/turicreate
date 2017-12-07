
add_library(iface INTERFACE)
target_sources(iface INTERFACE empty_1.cpp)

add_executable(main main.cpp)
target_link_libraries(main iface)
