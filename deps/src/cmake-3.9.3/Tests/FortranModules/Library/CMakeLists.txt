include_directories(${Library_MODDIR})
add_library(subdir_mods a.f90 b.f90)
add_executable(subdir_exe main.f90)
target_link_libraries(subdir_exe subdir_mods)

# Test module output directory if available.
if(CMAKE_Fortran_MODDIR_FLAG)
  set_target_properties(subdir_mods PROPERTIES
    Fortran_MODULE_DIRECTORY modules
    )
endif()
