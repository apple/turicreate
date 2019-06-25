project(perf)

make_library(perf OBJECT
  SOURCES
    tracepoint.cpp
  REQUIRES
    logger
  EXTERNAL_VISIBILITY
)
