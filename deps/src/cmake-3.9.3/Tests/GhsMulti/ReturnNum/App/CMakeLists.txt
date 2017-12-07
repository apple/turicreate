include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Lib)
add_executable(App Main.c)
target_link_libraries(App Lib)
target_compile_options(App PUBLIC "-non_shared")
