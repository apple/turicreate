enable_testing()
include(AndroidTestUtilities)

find_program(adb_executable adb)

set(ExternalData_URL_TEMPLATES
  "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"
  )

set(test_files "data/a.txt")

set(ANDROID 1)

android_add_test_data(setup_test
  FILES ${test_files}
  DEVICE_TEST_DIR "/data/local/tests/example1"
  DEVICE_OBJECT_STORE "/sdcard/.ExternalData/SHA")
