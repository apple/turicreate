# This executable directly depends on NoDepB, NoDepC, SixA and SixB. However,
# since NoDepB and NoDepC do not have explicit dependency information,
# and they depend on NoDepA, we have to manually specify that dependency.
link_libraries( NoDepB NoDepC NoDepA SixB SixA )

add_executable( exec ExecMain.c )

