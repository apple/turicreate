project(sframe_query_engine_test)

make_boost_test(sarray_source.cxx REQUIRES test_dependencies)
make_boost_test(sframe_source.cxx REQUIRES test_dependencies)
make_boost_test(project.cxx REQUIRES test_dependencies)
make_boost_test(transform.cxx REQUIRES test_dependencies)
make_boost_test(append.cxx REQUIRES test_dependencies)
make_boost_test(binary_transform.cxx REQUIRES test_dependencies)
make_boost_test(logical_filter.cxx REQUIRES test_dependencies)
make_boost_test(union.cxx REQUIRES test_dependencies)
make_boost_test(ternary_operator.cxx REQUIRES test_dependencies)

# The lambda test requires a pickled function without turicreate dependency
# make_boost_test(lambda_transform.cxx REQUIRES sframe sframe_query_engine)
