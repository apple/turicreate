project(unity_test_toolkits)

make_boost_test (fp_node.cxx REQUIRES unity_shared)
make_boost_test (fp_tree.cxx REQUIRES unity_shared)
make_boost_test (fp_results_tree.cxx REQUIRES unity_shared)
make_boost_test (rule_mining.cxx REQUIRES unity_shared)
make_boost_test (fp_growth.cxx REQUIRES unity_shared)
