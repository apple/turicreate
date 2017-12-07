project(lambda_test)

make_boost_test(worker_pool_test.cxx REQUIRES pylambda)

make_executable(dummy_worker
  SOURCES
    dummy_worker.cpp
  REQUIRES
    cppipc
    process
)
