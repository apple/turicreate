project(capi_test)

make_boost_test(capi_parameters.cxx REQUIRES test_dependencies)
make_boost_test(capi_flexible_type.cxx REQUIRES test_dependencies)
make_boost_test(capi_sarray.cxx REQUIRES test_dependencies)
make_boost_test(capi_sframe_csv.cxx REQUIRES test_dependencies)

# temporarily disabled because it fails intermittently
# make_boost_test(capi_sframe.cxx REQUIRES capi TuriCreate_static fileio)

make_boost_test(capi_models.cxx REQUIRES test_dependencies)
make_boost_test(capi_functions.cxx REQUIRES test_dependencies)
make_boost_test(capi_datetime.cxx REQUIRES test_dependencies)
make_boost_test(capi_ndarray.cxx REQUIRES test_dependencies)

# make_boost_test(basic_capi.cxx REQUIRES capi TuriCreate_static)

configure_file("json_test.json" "json_test.json" COPYONLY)
configure_file("json_test_empty.json" "json_test_empty.json" COPYONLY)
