project(sgraph_test)

make_boost_test(hilbert_par_for.cxx REQUIRES sgraph)
make_boost_test(sgraph_test.cxx REQUIRES sgraph)
make_boost_test(sgraph_vertex_apply_test.cxx REQUIRES sgraph)
make_boost_test(sgraph_engine_test.cxx REQUIRES sgraph)
make_boost_test(sgraph_triple_apply_test.cxx REQUIRES sgraph)
make_boost_test(sgraph_fast_triple_apply_test.cxx REQUIRES sgraph)
make_executable(sgraph_bench SOURCES sgraph_bench.cpp REQUIRES sgraph)
