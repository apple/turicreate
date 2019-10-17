project(unity_test_toolkits)

        make_boost_test(
            test_dc_data_iterator.cxx REQUIRES unity_shared_for_testing)