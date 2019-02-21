add_library(subdir_mods2 subdir.f90)
target_include_directories(subdir_mods2 INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
