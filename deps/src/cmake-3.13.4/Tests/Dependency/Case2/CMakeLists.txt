project(CASE2 C)

add_library(case2Foo1 STATIC foo1.c foo1b.c foo1c.c)
add_library(case2Foo2 STATIC foo2.c foo2b.c foo2c.c)
add_library(case2Foo3 STATIC foo3.c foo3b.c foo3c.c)
target_link_libraries(case2Foo1 case2Foo2)
target_link_libraries(case2Foo2 case2Foo3)
target_link_libraries(case2Foo3 case2Foo1)
set_property(TARGET case2Foo1 PROPERTY LINK_INTERFACE_MULTIPLICITY 3)

add_library(case2Bar1 STATIC bar1.c)
add_library(case2Bar2 STATIC bar2.c)
add_library(case2Bar3 STATIC bar3.c)
target_link_libraries(case2Bar1 case2Bar2 case2Foo1)
target_link_libraries(case2Bar2 case2Bar3)
target_link_libraries(case2Bar3 case2Bar1)

add_executable(case2Zot zot.c)
target_link_libraries(case2Zot case2Bar1)

#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
#set(CMAKE_LINK_DEPENDS_DEBUG_MODE 1)
