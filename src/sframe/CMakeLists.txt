project(sframe)

make_library(sframe OBJECT
  SOURCES
    dataframe.cpp
    sarray_index_file.cpp
    sframe_constants.cpp
    sframe_config.cpp
    sframe.cpp
    sframe_reader.cpp
    sframe_index_file.cpp
    parallel_csv_parser.cpp
    sframe_io.cpp
    shuffle.cpp
    csv_line_tokenizer.cpp
    sarray_v2_block_manager.cpp
    sarray_v2_type_encoding.cpp
    sarray_v2_block_writer.cpp
    sarray_sorted_buffer.cpp
    sarray_v2_encoded_block.cpp
    groupby.cpp
    groupby_aggregate.cpp
    groupby_aggregate_impl.cpp
    group_aggregate_value.cpp
    sframe_iterators.cpp
    csv_writer.cpp
    join.cpp
    join_impl.cpp
    unfair_lock.cpp
    sframe_rows.cpp
    testing_utils.cpp
    sframe_saving.cpp
    sframe_saving_impl.cpp
    sframe_compact.cpp
    rolling_aggregate.cpp
  REQUIRES
    random flexible_type fileio parallel lz4
    cancel_serverside_ops serialization libjson globals
  EXTERNAL_VISIBILITY    
)
