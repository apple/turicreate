project(sframe_query_engine_test)

make_boost_test(basic_end_to_end.cxx REQUIRES test_dependencies)
make_boost_test(optimizations.cxx REQUIRES test_dependencies)
make_boost_test(broadcast_queue.cxx REQUIRES test_dependencies)

subdirs(operators)
