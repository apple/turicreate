project(unity_toolkits)

make_boost_test(algo_utils.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(get_similar_tests.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(itemcf.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(new_user_tests.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(popularity.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(recommend_function.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(train_test_split.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(user_item_lists.cxx
  REQUIRES unity_shared_for_testing
)

make_boost_test(weird_segments.cxx
  REQUIRES unity_shared_for_testing
)


# Will enable these in a later pull request
make_boost_test(matrix_factorization.cxx
  REQUIRES unity_shared_for_testing
)
make_boost_test(factorization_machine.cxx
  REQUIRES unity_shared_for_testing
)


make_executable(test_memory_consumption
  SOURCES test_memory_consumption
  REQUIRES unity_shared_for_testing
)

make_executable(itemcf_stress_test
  SOURCES itemcf_stress_test.cpp
  REQUIRES unity_shared_for_testing
)
