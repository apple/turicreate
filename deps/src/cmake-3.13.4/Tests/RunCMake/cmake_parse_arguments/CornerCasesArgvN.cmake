include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

function(test1)
  cmake_parse_arguments(PARSE_ARGV 0
    mpref "" "" "MULTI")

  TEST(mpref_MULTI foo "foo\;bar")

  cmake_parse_arguments(PARSE_ARGV 1
    upref "" "" "MULTI")

  TEST(upref_UNPARSED_ARGUMENTS foo "foo\;bar")
endfunction()
test1(MULTI foo "foo\;bar")

function(test2)
  cmake_parse_arguments(PARSE_ARGV 0
    mpref "" "" "MULTI")

  TEST(mpref_MULTI "foo;" "bar;")

  cmake_parse_arguments(PARSE_ARGV 1
    upref "" "" "MULTI")

  TEST(upref_UNPARSED_ARGUMENTS "foo;" "bar;")
endfunction()
test2(MULTI "foo;" "bar;")

function(test3)
  cmake_parse_arguments(PARSE_ARGV 0
    mpref "" "" "MULTI")

  TEST(mpref_MULTI "[foo;]" "bar\\")

  cmake_parse_arguments(PARSE_ARGV 1
    upref "" "" "MULTI")

  TEST(upref_UNPARSED_ARGUMENTS "[foo;]" "bar\\")
endfunction()
test3(MULTI "[foo;]" "bar\\")

function(test4)
  cmake_parse_arguments(PARSE_ARGV 0
    mpref "" "" "MULTI")

  TEST(mpref_MULTI foo "bar;none")

  cmake_parse_arguments(PARSE_ARGV 1
    upref "" "" "MULTI")

  TEST(upref_UNPARSED_ARGUMENTS foo "bar;none")
endfunction()
test4(MULTI foo bar\\ none)
