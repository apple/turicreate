add_subdirectory(sub1)
subdirs(sub2)

add_custom_target(CustomSub)
add_library(InterfaceSub INTERFACE)
add_library(my::InterfaceSub ALIAS InterfaceSub)
