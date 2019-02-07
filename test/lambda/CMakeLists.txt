project(lambda_test)

make_boost_test(worker_pool_test.cxx REQUIRES unity_shared)

make_executable(dummy_worker
  SOURCES
    dummy_worker.cpp
  REQUIRES
    unity_shared
)
