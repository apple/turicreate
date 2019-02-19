project(capi_test)

make_boost_test(capi_parameters.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_flexible_type.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_sarray.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_sframe_csv.cxx REQUIRES unity_shared_for_testing)

# temporarily disabled because it fails intermittently
# make_boost_test(capi_sframe.cxx REQUIRES unity_shared_for_testing)

make_boost_test(capi_models.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_functions.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_datetime.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_ndarray.cxx REQUIRES unity_shared_for_testing)
make_boost_test(capi_visualization.cxx REQUIRES unity_shared_for_testing)

# make_boost_test(basic_capi.cxx REQUIRES capi unity_shared_for_testing)

configure_file("json_test.json" "json_test.json" COPYONLY)
configure_file("json_test_empty.json" "json_test_empty.json" COPYONLY)
