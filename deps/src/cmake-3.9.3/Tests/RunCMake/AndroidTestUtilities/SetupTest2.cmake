enable_testing()
include(AndroidTestUtilities)

add_custom_target(tests)
find_program(adb_executable adb)

set(ExternalData_URL_TEMPLATES
  "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"
  )
set(test_files
  "data/a.txt"
  "data/subfolder/b.txt"
  "data/subfolder/protobuffer.p"
  )

set(test_libs "data/subfolder/exampleLib.txt")

set(ANDROID 1)

android_add_test_data(setup_test
  FILES ${test_files}
  LIBS ${test_libs}
  DEVICE_TEST_DIR "/data/local/tests/example2"
  DEVICE_OBJECT_STORE "/sdcard/.ExternalData/SHA"
  NO_LINK_REGEX "\\.p$")

set_property(
  TARGET setup_test
  PROPERTY EXCLUDE_FROM_ALL 1)
add_dependencies(tests setup_test)
