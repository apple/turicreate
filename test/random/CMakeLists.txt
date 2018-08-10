project(random_test)
#make_boost_test(random_test.cxx REQUIRES test_dependencies)
make_executable(alias_benchmark SOURCES test_alias.cpp REQUIRES test_dependencies)