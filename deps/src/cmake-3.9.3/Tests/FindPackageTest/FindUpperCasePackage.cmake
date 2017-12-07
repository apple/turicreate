set(UCP_FOO TRUE)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)

find_package_handle_standard_args(UpperCasePackage REQUIRED_VARS UCP_FOO)
