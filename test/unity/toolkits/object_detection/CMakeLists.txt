project(unity_test_toolkits)

    add_subdirectory(one_shot_object_detection)

        make_boost_test(
            test_object_detector.cxx REQUIRES unity_shared_for_testing)

            make_boost_test(
                test_od_data_iterator.cxx REQUIRES unity_shared_for_testing)

                make_boost_test(
                    test_od_yolo.cxx REQUIRES unity_shared_for_testing)

                    make_boost_test(test_od_evaluation.cxx REQUIRES
                                        unity_shared_for_testing)

                        make_boost_test(test_od_serialization.cxx REQUIRES
                                            unity_shared_for_testing)
