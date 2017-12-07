project(unity_dml_test)

if(NOT WIN32)
if(NOT APPLE)

make_boost_test(dml_demo_tests.cxx
    REQUIRES distributed_toolkits_base)

make_boost_test(distributed_linear_regression_tests.cxx
    REQUIRES distributed_toolkits_base supervised_learning)

make_boost_test(distributed_linear_svm_tests.cxx
    REQUIRES distributed_toolkits_base supervised_learning)

make_boost_test(distributed_logistic_regression_tests.cxx
    REQUIRES distributed_toolkits_base supervised_learning)

make_boost_test(distributed_xgboost_tests.cxx
    REQUIRES distributed_toolkits_base supervised_learning)

make_boost_test(labelprop_tests.cxx
    REQUIRES distributed_toolkits_base)

make_boost_test(pagerank_tests.cxx
    REQUIRES distributed_toolkits_base)

make_boost_test(graph_ingress_tests.cxx
    REQUIRES distributed_toolkits_base)

make_copy_target(copy_dml_binaries
    TARGETS
      dml_shared
      distributed_demo
      distributed_graph_analytics
      distributed_supervised_learning
      dml_commander_startup
      dml_worker_startup)

endif() #APPLE
endif() # Windows
