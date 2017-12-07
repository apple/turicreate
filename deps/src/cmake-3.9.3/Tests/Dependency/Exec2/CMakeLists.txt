# Here, Eight depends on Seven, which has the same dependencies as Five.
# If the dependencies of Five are emitted, and then we attempt to emit the
# dependencies of Seven, then we find that they have already been done. So:
#  Original line:      Eight Five
#  Add deps of Five:   Eight Five Two ... NoDepA
# Now, we must make sure that Seven gets inserted between Five and Two, and
# not at the end. Unfortunately, if we get it wrong, the test will only
# fail on a platform where the link order makes a difference.
link_libraries( Eight Five )

add_executable( exec2 ExecMain.c )

