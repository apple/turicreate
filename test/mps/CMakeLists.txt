project(mps_test)

make_boost_test(layer_wrapper_tests.cxx
	REQUIRES unity_mps_graph
)
