file(GLOB DIRECTORIES LIST_DIRECTORIES true
  "${RunCMake_TEST_BINARY_DIR}/_install/FooExtension/*.*")

foreach(DIRECTORY IN LISTS DIRECTORIES)
  if(NOT DIRECTORY MATCHES "\\.foo$")
    message(SEND_ERROR "Extension does not match ${DIRECTORY}")
  endif()
endforeach()
