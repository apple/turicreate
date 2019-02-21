# pseudo find_module without specifying VERSION_VAR

set(FOOBAR TRUE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PseudoNoVersionVar REQUIRED_VARS FOOBAR)
