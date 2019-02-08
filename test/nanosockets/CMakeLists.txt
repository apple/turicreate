project(nanosockets)

make_boost_test(reqreptest.cxx REQUIRES unity_shared_for_testing)
#make_boost_test(pubsubtest.cxx REQUIRES unity_shared_for_testing)
