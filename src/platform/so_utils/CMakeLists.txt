project(so_utils)

if(NOT WIN32)
  add_library(so_utils OBJECT
    so_utils.cpp
  )
  add_dependencies(so_utils boost)
endif()
