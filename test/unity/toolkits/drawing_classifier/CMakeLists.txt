project(Turi)

make_boost_test(
    test_dc_data_iterator.cxx REQUIRES unity_shared_for_testing)

make_boost_test(
    test_drawing_classifier.cxx REQUIRES unity_shared_for_testing)

make_boost_test(
  test_dc_serialization.cxx REQUIRES unity_shared_for_testing)

make_boost_test(
  test_dc_evaluation.cxx REQUIRES unity_shared_for_testing)

make_boost_test(
  test_dc_prediction.cxx REQUIRES unity_shared_for_testing)
