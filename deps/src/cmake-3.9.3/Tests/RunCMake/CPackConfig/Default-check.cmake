include(${RunCMake_SOURCE_DIR}/check.cmake)

test_variable(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Foo\\Bar")
test_variable(CPACK_NSIS_PACKAGE_NAME "Bar\\Foo")

test_variable(CPACK_SOURCE_IGNORE_FILES
  "/CVS/;/\\.svn/;/\\.bzr/;/\\.hg/;/\\.git/;\\.swp$;\\.#;/#")
