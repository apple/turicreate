set(whitespaces_ "[\t\n\r ]*")

set(EXPECTED_FILES_COUNT "5")
set(EXPECTED_FILES_NAME_GENERATOR_SPECIFIC_FORMAT TRUE)

set(EXPECTED_FILE_1_NAME "Debuginfo")
set(EXPECTED_FILE_1_COMPONENT "applications")
set(EXPECTED_FILE_CONTENT_1_LIST "/usr;/usr/foo;/usr/foo/test_prog")
set(EXPECTED_FILE_2 "TestDinfo-pkg*-headers.rpm")
set(EXPECTED_FILE_CONTENT_2_LIST "/usr;/usr/bar;/usr/bar/CMakeLists.txt")
set(EXPECTED_FILE_3 "TestDinfo-pkg*-libs.rpm")
set(EXPECTED_FILE_CONTENT_3_LIST "/usr;/usr/bas;/usr/bas/libtest_lib.so")

set(EXPECTED_FILE_4_NAME "Debuginfo")
set(EXPECTED_FILE_4_COMPONENT "applications-debuginfo")
set(EXPECTED_FILE_CONTENT_4 ".*/src${whitespaces_}/src/src_1${whitespaces_}/src/src_1/main.cpp.*")
set(EXPECTED_FILE_5 "libs-DebugInfoPackage.rpm")
set(EXPECTED_FILE_CONTENT_5 ".*/src${whitespaces_}/src/src_1${whitespaces_}/src/src_1/test_lib.cpp.*")
