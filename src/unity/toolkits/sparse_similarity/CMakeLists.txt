project(unity_toolkits)

add_library(unity_sparse_similarity OBJECT
  sparse_similarity_lookup.cpp
  sliced_itemitem_matrix.cpp
)
add_dependencies(unity_sparse_similarity boost)