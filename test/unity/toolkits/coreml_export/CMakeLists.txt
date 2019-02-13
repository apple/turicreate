project(unity_test_toolkits)

make_boost_test(test_neural_nets_model_exporter.cxx
  REQUIRES
    unity_shared_for_testing
)
