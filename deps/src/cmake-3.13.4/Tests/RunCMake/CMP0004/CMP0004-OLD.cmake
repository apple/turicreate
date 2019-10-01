
cmake_minimum_required(VERSION 2.8.4)

cmake_policy(SET CMP0004 OLD)

add_library(foo SHARED empty.cpp)
add_library(bar SHARED empty.cpp)
add_library(bing SHARED empty.cpp)
add_library(bung SHARED empty.cpp)

cmake_policy(SET CMP0004 NEW)

add_library(bat SHARED empty.cpp)

target_link_libraries(foo "$<1: bar >")
target_link_libraries(bing "$<$<NOT:$<TARGET_POLICY:CMP0004>>: bar >")
target_link_libraries(bung "$<$<TARGET_POLICY:CMP0004>: bar >")

# The line below causes the error because the policy is NEW when bat
# is created.
target_link_libraries(bat "$<1: bar >")
