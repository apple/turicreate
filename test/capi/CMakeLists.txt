project(capi_test)

make_boost_test(capi_parameters.cxx REQUIRES turi_capi)
make_boost_test(capi_flexible_type.cxx REQUIRES turi_capi)
make_boost_test(capi_sarray.cxx REQUIRES turi_capi)
make_boost_test(capi_sframe.cxx REQUIRES turi_capi)
make_boost_test(capi_models.cxx REQUIRES turi_capi)
make_boost_test(capi_functions.cxx REQUIRES turi_capi)

# make_boost_test(basic_capi.cxx REQUIRES turi_capi)
