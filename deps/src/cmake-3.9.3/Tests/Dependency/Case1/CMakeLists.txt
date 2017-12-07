project(CASE1)

# The old anaylize lib depends stuff in cmTarget gets this case wrong.
# The cmComputeLinkDepends implementation gets it right.

add_library(case1a STATIC a.c)

add_library(case1b STATIC b.c b2.c)
target_link_libraries(case1b case1a)

add_library(case1c STATIC c.c c2.c)
target_link_libraries(case1c case1b)

add_library(case1d STATIC d.c)
target_link_libraries(case1d case1c)

add_executable(hello main.c)
target_link_libraries(hello case1c case1b case1d case1c)

