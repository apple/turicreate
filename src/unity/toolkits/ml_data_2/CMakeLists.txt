project(unity_toolkits)

add_library(unity_ml_data_2 OBJECT
  data_storage/ml_data_block_manager.cpp
  ml_data.cpp
  ml_data_column_modes.cpp
  ml_data_missing_values.cpp
  ml_data_sorting.cpp
  ml_data_setup.cpp

  metadata.cpp
  side_features.cpp

  data_storage/ml_data_row_format.cpp
  data_storage/ml_data_row_translation.cpp
  data_storage/internal_metadata.cpp
  data_storage/util.cpp

  indexing/column_indexer.cpp
  indexing/column_unique_indexer.cpp
  statistics/column_statistics.cpp
  statistics/basic_column_statistics.cpp
  iterators/ml_data_iterator_base.cpp
  iterators/ml_data_block_iterator.cpp
  iterators/composite_row_type.cpp

  sframe_index_mapping.cpp

  testing_utils.cpp

  row_slicing_utilities.cpp
)
add_dependencies(unity_ml_data_2 boost)
