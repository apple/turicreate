project(CASE3 C)

add_library(case3Foo1 STATIC foo1.c foo1b.c)
add_library(case3Foo2 STATIC foo2.c)

add_executable(case3Bar bar.c)
target_link_libraries(case3Bar case3Foo1 case3Foo2 case3Foo1)

#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
#set(CMAKE_LINK_DEPENDS_DEBUG_MODE 1)
