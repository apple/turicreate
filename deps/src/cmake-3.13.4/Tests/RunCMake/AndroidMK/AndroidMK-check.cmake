# This file does a regex file compare on the generated
# Android.mk files from the AndroidMK test

macro(compare_file_to_expected file expected_file)
  file(READ "${file}" ANDROID_MK)
  # clean up new lines
  string(REGEX REPLACE "\r\n" "\n" ANDROID_MK "${ANDROID_MK}")
  string(REGEX REPLACE "\n+$" "" ANDROID_MK "${ANDROID_MK}")
  # read in the expected regex file
  file(READ "${expected_file}" expected)
  # clean up new lines
  string(REGEX REPLACE "\r\n" "\n" expected "${expected}")
  string(REGEX REPLACE "\n+$" "" expected "${expected}")
  # compare the file to the expected regex and if there is not a match
  # put an error message in RunCMake_TEST_FAILED
  if(NOT "${ANDROID_MK}" MATCHES "${expected}")
    set(RunCMake_TEST_FAILED
      "${file} does not match ${expected_file}:

Android.mk contents = [\n${ANDROID_MK}\n]
Expected = [\n${expected}\n]")
  endif()
endmacro()

compare_file_to_expected(
"${RunCMake_BINARY_DIR}/AndroidMK-build/Android.mk"
"${RunCMake_TEST_SOURCE_DIR}/expectedBuildAndroidMK.txt")
compare_file_to_expected(
"${RunCMake_BINARY_DIR}/AndroidMK-build/CMakeFiles/Export/share/ndk-modules/Android.mk"
"${RunCMake_TEST_SOURCE_DIR}/expectedInstallAndroidMK.txt")
