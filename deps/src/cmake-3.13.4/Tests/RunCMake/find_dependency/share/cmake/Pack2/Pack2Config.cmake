include(CMakeFindDependencyMacro)

find_dependency(Pack1 PATHS ${CMAKE_CURRENT_LIST_DIR}/..)

add_library(Pack2::Lib INTERFACE IMPORTED)
set_target_properties(Pack2::Lib PROPERTIES INTERFACE_LINK_LIBRARIES Pack1::Lib)
