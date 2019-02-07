project(unity_test_toolkits)

make_boost_test(test_object_detector.cxx REQUIRES unity_shared)

make_boost_test(test_od_data_iterator.cxx REQUIRES unity_shared)

make_boost_test(test_od_yolo.cxx
  REQUIRES
    unity_shared
)

make_boost_test(test_od_evaluation.cxx REQUIRES unity_toolkits)
