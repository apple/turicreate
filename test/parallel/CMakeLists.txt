project(parallel_test)


make_boost_test(thread_tools.cxx REQUIRES parallel)
make_boost_test(atomic_ops.cxx REQUIRES parallel util)
if(NOT WIN32)
make_boost_test(lambda_omp_test.cxx REQUIRES parallel)
endif()
