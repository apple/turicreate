if(RunCMake_SUBTEST_SUFFIX MATCHES "^(none|good(_multi)?|invalid_good)$"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "stage_and_package")
  set(EXPECTED_FILES_COUNT "1")
  set(EXPECTED_FILE_CONTENT_1_LIST "/share;/share/cpack-test;/share/cpack-test/f1.txt;/share/cpack-test/f2.txt;/share/cpack-test/f3.txt;/share/cpack-test/f4.txt")
else()
  set(EXPECTED_FILES_COUNT "0")
endif()
