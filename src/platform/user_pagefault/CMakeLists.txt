project(user_pagefault)

if (NOT WIN32)
  add_library(user_pagefault OBJECT 
    user_pagefault.cpp pagefile.cpp type_heuristic_encode.cpp 
  )
  add_dependencies(user_pagefault boost)
endif()
