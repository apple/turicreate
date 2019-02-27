file(READ "${RunCMake_TEST_BINARY_DIR}/TARGET_NAME_IF_EXISTS-generated.txt" content)

set(expected "foo")
if(NOT content STREQUAL expected)
  set(RunCMake_TEST_FAILED "actual content:\n [[${content}]]\nbut expected:\n [[${expected}]]")
endif()
