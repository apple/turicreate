
include(CMakeFindDependencyMacro)

find_dependency(Pack2 2.3)
find_dependency(Pack3)

add_library(Pack1::Lib INTERFACE IMPORTED)
set_property(TARGET Pack1::Lib PROPERTY INTERFACE_COMPILE_DEFINITIONS HAVE_PACK1)
set_property(TARGET Pack1::Lib PROPERTY INTERFACE_LINK_LIBRARIES Pack2::Lib Pack3::Lib)
