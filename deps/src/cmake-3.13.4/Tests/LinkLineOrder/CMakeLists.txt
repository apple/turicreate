project( LinkLineOrder )

# This tests ensures that the order of libraries are preserved when
# they don't have dependency information, even if they are deep in the
# dependency tree.

# NoDepC depends on NoDepA which depends on NoDepB. NoDepE and NoDepF
# are dependent on each other (recursive dependency). However, CMake
# has no information about these libraries except for the order they
# are specified in One. We must make sure we don't lose that.

add_library( NoDepA NoDepA.c )
add_library( NoDepB NoDepB.c )
add_library( NoDepC NoDepC.c )
add_library( NoDepE NoDepE.c )
add_library( NoDepF NoDepF.c )

add_library( One One.c )
target_link_libraries( One NoDepC NoDepA NoDepB NoDepE NoDepF NoDepE )

add_executable( Exec1 Exec1.c )
target_link_libraries( Exec1 One )


# Similar situation as One, except at a different level of the
# dependency tree. This makes sure that the order is presevered
# everywhere in the graph.
add_library( NoDepX NoDepX.c )
add_library( NoDepY NoDepY.c )
add_library( NoDepZ NoDepZ.c )

add_library( Two Two.c )
target_link_libraries( Two One NoDepZ NoDepX NoDepY )

add_executable( Exec2 Exec2.c )
target_link_libraries( Exec2 Two )
