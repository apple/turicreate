add_executable(mytest test.cpp)

file(GENERATE OUTPUT runtest_info.cmake CONTENT [[
set(EXPECTED_MYTEST_NAME "$<TARGET_FILE_NAME:mytest>")
]])

set_property(INSTALL bar/test.cpp PROPERTY CPACK_TEST_PROP $<TARGET_FILE_NAME:mytest>)
