file(READ "${RunCMake_TEST_BINARY_DIR}/TARGET_NAME_IF_EXISTS-not-a-target-generated.txt" content)

if(content)
  set(RunCMake_TEST_FAILED "actual content:\n [[${content}]]\nbut expected an empty string")
endif()
