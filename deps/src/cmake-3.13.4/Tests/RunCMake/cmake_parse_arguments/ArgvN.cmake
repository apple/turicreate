include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

function(test1)
  cmake_parse_arguments(PARSE_ARGV 0
    pref "OPT1;OPT2" "SINGLE1;SINGLE2" "MULTI1;MULTI2")

  TEST(pref_OPT1 TRUE)
  TEST(pref_OPT2 FALSE)
  TEST(pref_SINGLE1 "foo;bar")
  TEST(pref_SINGLE2 UNDEFINED)
  TEST(pref_MULTI1 bar foo bar)
  TEST(pref_MULTI2 UNDEFINED)
  TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)
endfunction()
test1(OPT1 SINGLE1 "foo;bar" MULTI1 bar foo bar)

function(test2 arg1)
  cmake_parse_arguments(PARSE_ARGV 1
    pref "OPT1;OPT2" "SINGLE1;SINGLE2" "MULTI1;MULTI2")

  TEST(arg1 "first named")
  TEST(pref_OPT1 TRUE)
  TEST(pref_OPT2 FALSE)
  TEST(pref_SINGLE1 "foo;bar")
  TEST(pref_SINGLE2 UNDEFINED)
  TEST(pref_MULTI1 bar "foo;bar")
  TEST(pref_MULTI2 UNDEFINED)
  TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)
endfunction()
test2("first named" OPT1 SINGLE1 "foo;bar" MULTI1 bar "foo;bar")

function(test3 arg1)
  cmake_parse_arguments(PARSE_ARGV 0
    pref "" "" "")

  TEST(pref_UNPARSED_ARGUMENTS "foo;bar" dog cat)
endfunction()
test3("foo;bar" dog cat)
