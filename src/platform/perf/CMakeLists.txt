project(perf)

make_library(perf
  SOURCES
  tracepoint.cpp
  REQUIRES
  logger 
    EXTERNAL_VISIBILITY
)
