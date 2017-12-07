enable_testing()
include(AndroidTestUtilities)

add_custom_target(tests)
find_program(adb_executable adb)

set(ExternalData_URL_TEMPLATES
  "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"
  )
set(test_dir "/data/local/tests/example3")
set(test_files
  "data/a.txt"
  "data/subfolder/b.txt"
  )
set(test_libs "libs/exampleLib.txt")
set(files_dest "${test_dir}/storage_folder")
set(libs_dest "${test_dir}/lib/lib/lib")

set(ANDROID 1)

android_add_test_data(setup_test
  FILES ${test_files}
  LIBS ${test_libs}
  FILES_DEST ${files_dest}
  LIBS_DEST ${libs_dest}
  DEVICE_TEST_DIR "/data/local/tests/example3"
  DEVICE_OBJECT_STORE "/sdcard/.ExternalData/SHA"
  NO_LINK_REGEX "\\.p$")

set_property(
  TARGET setup_test
  PROPERTY EXCLUDE_FROM_ALL 1)
add_dependencies(tests setup_test)
