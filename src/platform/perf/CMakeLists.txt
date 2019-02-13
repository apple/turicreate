project(perf)

add_library(perf OBJECT
  tracepoint.cpp
)
add_dependencies(perf boost)
