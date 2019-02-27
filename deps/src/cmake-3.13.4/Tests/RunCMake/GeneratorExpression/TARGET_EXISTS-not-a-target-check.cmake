file(READ "${RunCMake_TEST_BINARY_DIR}/TARGET_EXISTS-not-a-target-generated.txt" content)

set(expected "0")
if(NOT content STREQUAL expected)
  set(RunCMake_TEST_FAILED "actual content:\n [[${content}]]\nbut expected:\n [[${expected}]]")
endif()
