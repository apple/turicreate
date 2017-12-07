# Here, Five already has it's immediate dependency, Two satisfied. We must
# make sure Two gets output anyway, because Eight indirectly depends on it.
link_libraries( Five Two Eight Five )

add_executable( exec3 ExecMain.c )

