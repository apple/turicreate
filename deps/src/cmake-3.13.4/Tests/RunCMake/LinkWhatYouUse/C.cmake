enable_language(C)
set(CMAKE_LINK_WHAT_YOU_USE TRUE)
add_executable(main main.c)
target_link_libraries(main m)
