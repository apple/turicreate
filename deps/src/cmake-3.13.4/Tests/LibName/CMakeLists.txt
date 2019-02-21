project(LibName)
# this is a test to make sure that relative path
# LIBRARY_OUTPUT_PATH and EXECUTABLE_OUTPUT_PATH work
set(LIBRARY_OUTPUT_PATH  lib)
set(EXECUTABLE_OUTPUT_PATH lib)

add_library(bar SHARED bar.c)

add_library(foo SHARED foo.c)
target_link_libraries(foo bar)

add_executable(foobar foobar.c)
target_link_libraries(foobar foo)
if(UNIX)
  target_link_libraries(foobar -L/usr/local/lib)
endif()


# check with lib version

add_library(verFoo SHARED foo.c)
target_link_libraries(verFoo bar)
set_target_properties(verFoo PROPERTIES VERSION 3.1.4 SOVERSION 3)

add_executable(verFoobar foobar.c)
target_link_libraries(verFoobar verFoo)
