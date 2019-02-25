add_executable(SubDirA SubDirA.c)

# Link to a target imported in this directory that would not normally
# be visible to the directory in which TopDir is defined.
target_link_libraries(TopDir PUBLIC SameNameImported)

# Link SubDirA to a target imported in this directory that has the same
# name as a target imported in SubDirB's directory.  SubDirB will also
# tell us to link its copy.  At compile time we verify both are linked.
target_link_libraries(SubDirA PRIVATE SameNameImported)

# Import a target with the same name as a target imported in SubDirB.
# Distinguish this copy by having a unique usage requirement.
add_library(SameNameImported IMPORTED INTERFACE)
target_compile_definitions(SameNameImported INTERFACE DEF_SameNameImportedSubDirA)
