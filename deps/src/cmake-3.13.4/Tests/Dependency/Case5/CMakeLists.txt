project(CASE5 C)

add_library(case5Foo SHARED foo.c)
add_library(case5Bar STATIC bar.c)
target_link_libraries(case5Bar case5Foo)

add_executable(case5 main.c)
target_link_libraries(case5 case5Foo case5Bar)
