project(unity_test)


make_boost_test(unity_sarray.cxx REQUIRES test_dependencies)
make_boost_test(unity_sarray_lazy_eval.cxx REQUIRES test_dependencies)
make_boost_test(unity_sframe.cxx REQUIRES test_dependencies)
make_boost_test(unity_sframe_lazy_eval.cxx REQUIRES test_dependencies)
make_boost_test(unity_sgraph.cxx REQUIRES test_dependencies)
make_boost_test(flex_dict_view.cxx REQUIRES test_dependencies)
make_boost_test(unity_sketch.cxx REQUIRES test_dependencies)
make_boost_test(unity_toolkit.cxx REQUIRES test_dependencies)
make_boost_test(gl_sarray.cxx REQUIRES test_dependencies)
make_boost_test(gl_sframe.cxx REQUIRES test_dependencies)
make_boost_test(gl_sgraph.cxx REQUIRES test_dependencies)
make_boost_test(gl_gframe.cxx REQUIRES test_dependencies)
make_boost_test(image_util.cxx REQUIRES test_dependencies)
subdirs(
  toolkits
  )
