project(random_test)
#make_boost_test(random_test.cxx REQUIRES unity_shared)
make_executable(alias_benchmark SOURCES test_alias.cpp REQUIRES unity_shared)
