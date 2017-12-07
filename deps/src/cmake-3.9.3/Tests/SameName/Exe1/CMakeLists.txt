# a target with the same name as a target in a different dir
add_executable(mytest_exe conly.c)
set_target_properties(mytest_exe PROPERTIES OUTPUT_NAME mytest)
target_link_libraries(mytest_exe mytest)

# and two targets in the same dir with the same name
add_library(mytest2 ../Lib1/libc1.c)

add_executable(mytest2_exe conly.c)
set_target_properties(mytest2_exe PROPERTIES OUTPUT_NAME mytest2)
target_link_libraries(mytest2_exe mytest2)
