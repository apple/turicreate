add_library(empty UNKNOWN IMPORTED)
add_custom_target(custom COMMAND echo $<TARGET_PDB_FILE:empty>)
