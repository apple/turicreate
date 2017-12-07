enable_language(CXX)

add_subdirectory(ExcludeFromAll EXCLUDE_FROM_ALL)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE foo)
