include_directories(${Library_MODDIR})
include_directories(${External_BINARY_DIR})
link_directories(${External_BINARY_DIR})

add_executable(subdir_exe2 main.f90)
target_link_libraries(subdir_exe2 subdir_mods subdir_mods2)
add_dependencies(subdir_exe2 ExternalTarget)
target_link_libraries(subdir_exe2 myext)
