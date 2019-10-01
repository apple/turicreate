
add_library(foo STATIC empty.cpp)
add_library(bar STATIC empty.cpp)
target_link_libraries(foo $<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,anything>:bar>)
