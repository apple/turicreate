project(Turi)

if(NOT WIN32)
  make_library(so_utils OBJECT
    SOURCES
      so_utils.cpp
    REQUIRES
      dl
  )
endif()
