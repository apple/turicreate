cmake_minimum_required(VERSION 3.5)
project(demo C)

add_library(libdemo
  test.c
  subfolder_test.c
  subfolder_test_0.c
  "subfolder/test.c"
)

add_executable(demo main.c)
target_link_libraries(demo libdemo)
if(GHSMULTI)
    target_compile_options(demo PUBLIC "-non_shared")
endif()
