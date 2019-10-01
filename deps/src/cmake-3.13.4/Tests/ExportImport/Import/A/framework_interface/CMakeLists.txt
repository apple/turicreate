
add_library(exp_framework_test framework_test.cpp)
get_target_property(exp_loc exp_testLib4 LOCATION)
target_link_libraries(exp_framework_test ${exp_loc})


add_library(bld_framework_test framework_test.cpp)
get_target_property(bld_loc bld_testLib4 LOCATION)
target_link_libraries(bld_framework_test ${bld_loc})
