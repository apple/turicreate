project(sframe_query_engine_test)

make_boost_test(basic_end_to_end.cxx REQUIRES unity_shared)
make_boost_test(optimizations.cxx REQUIRES unity_shared)
make_boost_test(broadcast_queue.cxx REQUIRES unity_shared)

subdirs(operators)
