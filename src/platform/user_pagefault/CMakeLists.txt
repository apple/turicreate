project(user_pagefault)

if (NOT WIN32)
  make_library(user_pagefault OBJECT 
    SOURCES
      user_pagefault.cpp pagefile.cpp type_heuristic_encode.cpp 
    REQUIRES
      fileio lz4 minipsutil_static sframe
  )
endif()
