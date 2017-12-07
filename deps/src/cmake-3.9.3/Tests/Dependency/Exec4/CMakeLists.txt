# Even though Five's dependency on Two is explicitly satisfied, Two
# must be emitted again in order to satisfy a cyclic dependency on Three.
link_libraries( Five Two Five )

add_executable( exec4 ExecMain.c )

