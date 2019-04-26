project(sgraph)

make_library(sgraph OBJECT
  SOURCES
    sgraph.cpp
    sgraph_triple_apply.cpp
    sgraph_fast_triple_apply.cpp
    sgraph_io.cpp
    sgraph_constants.cpp
  REQUIRES
    flexible_type sframe pylambda sparsehash
  EXTERNAL_VISIBILITY
)
