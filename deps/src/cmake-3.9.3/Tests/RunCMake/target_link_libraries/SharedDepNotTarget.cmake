enable_language(C)
set(CMAKE_LINK_DEPENDENT_LIBRARY_DIRS 1)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
add_library(imported SHARED IMPORTED)
set_target_properties(imported PROPERTIES
  IMPORTED_LOCATION "imported"
  IMPORTED_LINK_DEPENDENT_LIBRARIES "/path/to/libSharedDep.so"
  )
add_executable(empty empty.c)
target_link_libraries(empty imported)
