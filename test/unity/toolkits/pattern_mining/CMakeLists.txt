project(unity_test_toolkits)

make_boost_test (fp_node.cxx REQUIRES test_dependencies)
make_boost_test (fp_tree.cxx REQUIRES test_dependencies)
make_boost_test (fp_results_tree.cxx REQUIRES test_dependencies)
make_boost_test (rule_mining.cxx REQUIRES test_dependencies)
make_boost_test (fp_growth.cxx REQUIRES test_dependencies)
