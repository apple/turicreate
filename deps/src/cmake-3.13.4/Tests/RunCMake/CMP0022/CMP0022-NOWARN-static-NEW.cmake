
project(CMP0022-NOWARN-static-NEW)

cmake_policy(SET CMP0022 NEW)

add_library(foo STATIC empty_vs6_1.cpp)
add_library(bar STATIC empty_vs6_2.cpp)
add_library(bat STATIC empty_vs6_3.cpp)
target_link_libraries(foo bar)
# The last element here needs to contain a space so that it is a single
# element which is not a valid target name. As bar is a STATIC library,
# this tests that the LINK_ONLY generator expression is not used for
# that element, creating an error.
target_link_libraries(bar LINK_PRIVATE bat "-lz -lm")
