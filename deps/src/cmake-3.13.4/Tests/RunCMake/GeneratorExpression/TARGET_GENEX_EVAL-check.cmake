file(READ "${RunCMake_TEST_BINARY_DIR}/TARGET_GENEX_EVAL-generated.txt" content)

set(expected "BEFORE_PROPERTY1_AFTER")
if(NOT content STREQUAL expected)
  set(RunCMake_TEST_FAILED "actual content:\n [[${content}]]\nbut expected:\n [[${expected}]]")
endif()
