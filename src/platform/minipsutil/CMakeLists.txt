make_library(minipsutil
  SOURCES
  minipsutil.c SHARED
    EXTERNAL_VISIBILITY
  )

make_library(minipsutil_static
  SOURCES
  minipsutil.c 
  )

