
cmake_minimum_required(VERSION 2.8.4)

cmake_policy(SET CMP0004 NEW)

add_library(foo SHARED empty.cpp)
add_library(bar SHARED empty.cpp)
add_library(bat SHARED empty.cpp)

# The negation here avoids the error.
target_link_libraries(foo "$<$<NOT:$<TARGET_POLICY:CMP0004>>: bar >")

# The below line causes the error.
target_link_libraries(foo "$<$<TARGET_POLICY:CMP0004>: bat >")
