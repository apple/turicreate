project(lambda_test)

make_boost_test(worker_pool_test.cxx REQUIRES test_dependencies)

make_executable(dummy_worker
  SOURCES
    dummy_worker.cpp
  REQUIRES
    test_dependencies
)
