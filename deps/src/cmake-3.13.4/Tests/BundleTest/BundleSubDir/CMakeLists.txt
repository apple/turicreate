add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  COMMAND /bin/cp
  ARGS "${BundleTest_SOURCE_DIR}/randomResourceFile.plist.in"
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist")

set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
  )

set_source_files_properties(
  "${BundleTest_SOURCE_DIR}/SomeRandomFile.txt"
  "${BundleTest_SOURCE_DIR}/../../README.rst"
  PROPERTIES
  MACOSX_PACKAGE_LOCATION MacOS
  )

add_executable(SecondBundle
  MACOSX_BUNDLE
  "${BundleTest_SOURCE_DIR}/BundleTest.cxx"
  "${BundleTest_SOURCE_DIR}/SomeRandomFile.txt"
  "${BundleTest_SOURCE_DIR}/../../README.rst"
  "${CMAKE_CURRENT_BINARY_DIR}/randomResourceFile.plist"
  )
target_link_libraries(SecondBundle BundleTestLib)

# Test bundle installation.
install(TARGETS SecondBundle DESTINATION Applications)

# Test whether bundles respect the output name.  Since the library is
# installed into a location that uses this output name this will fail if the
# bundle does not respect the name.  Also the executable will not be found by
# the test driver if this does not work.
set_target_properties(SecondBundle PROPERTIES OUTPUT_NAME SecondBundleExe)
