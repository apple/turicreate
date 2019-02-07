project(parallel_test)


make_boost_test(thread_tools.cxx REQUIRES unity_shared)
make_boost_test(atomic_ops.cxx REQUIRES unity_shared)
if(NOT WIN32)
    make_boost_test(lambda_omp_test.cxx REQUIRES unity_shared)
endif()
