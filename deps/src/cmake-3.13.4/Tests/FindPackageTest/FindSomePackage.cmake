set(SOP_FOO TRUE)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)

find_package_handle_standard_args(SomePackage REQUIRED_VARS SOP_FOO)
