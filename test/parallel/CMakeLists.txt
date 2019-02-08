project(parallel_test)


make_boost_test(thread_tools.cxx REQUIRES unity_shared_for_testing)
make_boost_test(atomic_ops.cxx REQUIRES unity_shared_for_testing)
if(NOT WIN32)
    make_boost_test(lambda_omp_test.cxx REQUIRES unity_shared_for_testing)
endif()
