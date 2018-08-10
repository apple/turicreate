project(parallel_test)


make_boost_test(thread_tools.cxx REQUIRES test_dependencies)
make_boost_test(atomic_ops.cxx REQUIRES test_dependencies)
if(NOT WIN32)
make_boost_test(lambda_omp_test.cxx REQUIRES test_dependencies)
endif()
