project(unity_toolkits)

make_library(unity_sparse_similarity OBJECT
  SOURCES
    sparse_similarity_lookup.cpp
    sliced_itemitem_matrix.cpp
  REQUIRES
    sframe
    unity_core
)
