project(ml_data)

make_boost_test(dml_basic_storage.cxx REQUIRES ml_data)
make_boost_test(dml_numerics.cxx REQUIRES ml_data)
make_boost_test(dml_untranslated_columns.cxx REQUIRES ml_data)
make_boost_test(dml_reindexing.cxx REQUIRES ml_data)
# KNOWN FAILURE
# make_boost_test(dml_stats_merge_test.cxx REQUIRES ml_data)
make_boost_test(dml_metadata_api.cxx REQUIRES ml_data)
make_boost_test(dml_schema_errors.cxx REQUIRES ml_data)
make_boost_test(dml_test_row_bounds.cxx REQUIRES ml_data)
make_boost_test(dml_sorted_columns.cxx REQUIRES ml_data)
