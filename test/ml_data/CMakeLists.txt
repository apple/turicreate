project(ml_data)

make_boost_test(dml_basic_storage.cxx REQUIRES test_dependencies)
make_boost_test(dml_numerics.cxx REQUIRES test_dependencies)
make_boost_test(dml_untranslated_columns.cxx REQUIRES test_dependencies)
make_boost_test(dml_reindexing.cxx REQUIRES test_dependencies)
# KNOWN FAILURE
# make_boost_test(dml_stats_merge_test.cxx REQUIRES test_dependencies)
make_boost_test(dml_metadata_api.cxx REQUIRES test_dependencies)
make_boost_test(dml_schema_errors.cxx REQUIRES test_dependencies)
make_boost_test(dml_test_row_bounds.cxx REQUIRES test_dependencies)
make_boost_test(dml_sorted_columns.cxx REQUIRES test_dependencies)
