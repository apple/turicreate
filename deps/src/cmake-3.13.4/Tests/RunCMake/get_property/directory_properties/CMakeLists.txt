add_subdirectory(sub1)
subdirs(sub2)

add_custom_target(CustomSub)
add_library(InterfaceSub INTERFACE)
add_library(my::InterfaceSub ALIAS InterfaceSub)

add_test(Sub/test1 COMMAND "${CMAKE_COMMAND}" -E echo "Sub/test1")
add_test(Sub/test2 COMMAND "${CMAKE_COMMAND}" -E echo "Sub/test2")
