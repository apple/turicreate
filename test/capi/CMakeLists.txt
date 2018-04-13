project(capi_test)

make_boost_test(capi_parameters.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_flexible_type.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_sarray.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_sframe.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_models.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_functions.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_datetime.cxx REQUIRES TuriCreate_static)
make_boost_test(capi_ndarray.cxx REQUIRES TuriCreate_static)

# make_boost_test(basic_capi.cxx REQUIRES TuriCreate_static)

configure_file("json_test.json" "json_test.json" COPYONLY)
