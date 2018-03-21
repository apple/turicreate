project(capi_test)

make_boost_test(capi_parameters.cxx REQUIRES turi_capi)
make_boost_test(capi_flexible_type.cxx REQUIRES turi_capi)
make_boost_test(capi_sarray.cxx REQUIRES turi_capi)
make_boost_test(capi_sframe.cxx ../../src/unity/extensions/random_sframe_generation.cpp REQUIRES turi_capi)
make_boost_test(capi_models.cxx REQUIRES turi_capi)
make_boost_test(capi_functions.cxx REQUIRES turi_capi)
make_boost_test(capi_datetime.cxx REQUIRES turi_capi)
make_boost_test(capi_ndarray.cxx REQUIRES turi_capi)

# make_boost_test(basic_capi.cxx REQUIRES turi_capi)

configure_file("json_test.json" "json_test.json" COPYONLY)
