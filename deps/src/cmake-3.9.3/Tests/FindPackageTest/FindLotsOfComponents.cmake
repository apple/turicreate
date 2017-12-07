set(LOC_FOO TRUE)

set(LotsOfComponents_AComp_FOUND TRUE)
set(LotsOfComponents_BComp_FOUND FALSE)
set(LotsOfComponents_CComp_FOUND TRUE)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)

find_package_handle_standard_args(LotsOfComponents REQUIRED_VARS LOC_FOO
                                                   HANDLE_COMPONENTS)
