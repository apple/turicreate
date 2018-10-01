project(sframe_query_engine_test)

make_boost_test(basic_end_to_end.cxx REQUIRES sframe sframe_query_engine)
make_boost_test(optimizations.cxx REQUIRES sframe sframe_query_engine)
make_boost_test(broadcast_queue.cxx REQUIRES fileio sframe_query_engine)

subdirs(operators)
