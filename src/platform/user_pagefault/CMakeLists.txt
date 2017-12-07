project(user_pagefault)

if (WIN32)
else()
make_library(user_pagefault SOURCES 
  user_pagefault.cpp pagefile.cpp type_heuristic_encode.cpp 
  REQUIRES fileio lz4 minipsutil_static sframe)
endif()
