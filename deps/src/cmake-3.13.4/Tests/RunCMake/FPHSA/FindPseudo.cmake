# pseudo find_module

set(FOOBAR TRUE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pseudo REQUIRED_VARS FOOBAR VERSION_VAR Pseudo_VERSION)
