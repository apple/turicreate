enable_language(C)

add_executable(top empty.c)
add_subdirectory(CMP0079-link)
get_property(libs TARGET top PROPERTY LINK_LIBRARIES)
message(STATUS "LINK_LIBRARIES='${libs}'")
