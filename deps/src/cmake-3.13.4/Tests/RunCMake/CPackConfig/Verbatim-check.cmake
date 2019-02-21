include(${RunCMake_SOURCE_DIR}/check.cmake)

test_variable(CPACK_BACKSLASH "\\\\")
test_variable(CPACK_QUOTE "a\" b \"c")
test_variable(CPACK_DOLLAR "a\${NOTHING}b")

# make sure the default for this is still set correctly with
# CPACK_VERBATIM_VARIABLES on
test_variable(CPACK_SOURCE_IGNORE_FILES
  "/CVS/;/\\.svn/;/\\.bzr/;/\\.hg/;/\\.git/;\\.swp$;\\.#;/#")
