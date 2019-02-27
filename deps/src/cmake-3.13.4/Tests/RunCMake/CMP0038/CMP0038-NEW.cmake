
cmake_policy(SET CMP0038 NEW)
add_library(self_link empty.cpp)
target_link_libraries(self_link self_link)
