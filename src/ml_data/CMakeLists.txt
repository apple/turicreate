project(unity)

make_library(ml_data OBJECT
  SOURCES
    ml_data.cpp
    metadata.cpp
    ml_data_column_modes.cpp
    data_storage/ml_data_block_manager.cpp
    data_storage/ml_data_row_format.cpp
    data_storage/ml_data_row_translation.cpp
    data_storage/internal_metadata.cpp
    data_storage/util.cpp
    column_indexer.cpp
    column_statistics.cpp
    ml_data_iterator.cpp
    row_reference.cpp
    testing_utils.cpp
  REQUIRES
    unity_core eigen
    EXTERNAL_VISIBILITY
)
