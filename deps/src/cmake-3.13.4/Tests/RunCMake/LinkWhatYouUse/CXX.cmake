enable_language(CXX)
set(CMAKE_LINK_WHAT_YOU_USE TRUE)
add_executable(main main.cxx)
target_link_libraries(main m)
