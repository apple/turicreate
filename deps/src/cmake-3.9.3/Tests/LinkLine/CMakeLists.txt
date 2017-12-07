project( LinkLine )

# Makes sure that the library order as specified by the user are
# unchanged by dependency analysis, etc.  libOne and libTwo are
# dependent on each other. The link line should be -lOne -lTwo -lOne.

add_library( One One.c )
add_library( Two Two.c )

link_libraries( One Two )
add_executable( LinkLine Exec.c )
link_libraries( One )
