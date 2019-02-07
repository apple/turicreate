project(sgraph)

add_library(sgraph OBJECT
  sgraph.cpp
  sgraph_triple_apply.cpp
  sgraph_fast_triple_apply.cpp
  sgraph_io.cpp
  sgraph_constants.cpp
)
add_dependencies(sgraph boost)