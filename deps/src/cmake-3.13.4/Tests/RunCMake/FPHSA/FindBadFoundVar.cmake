set(BFV_FOO TRUE)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(BadFoundVar REQUIRED_VARS BFV_FOO
                                              FOUND_VAR badfoundvar_FOUND )
