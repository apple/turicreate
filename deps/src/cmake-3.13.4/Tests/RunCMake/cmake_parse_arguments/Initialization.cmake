include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

# unparsed arguments
cmake_parse_arguments(pref "" "" "")
TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)

cmake_parse_arguments(pref "" "" "" FOO)
TEST(pref_UNPARSED_ARGUMENTS "FOO")
cmake_parse_arguments(pref "" "" "" FOO BAR)
TEST(pref_UNPARSED_ARGUMENTS "FOO;BAR")
cmake_parse_arguments(pref "" "" "")
TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)


# options
cmake_parse_arguments(pref "OPT1" "" "")
TEST(pref_OPT1 FALSE)

cmake_parse_arguments(pref "OPT1;OPT2" "" "")
TEST(pref_OPT1 FALSE)
TEST(pref_OPT2 FALSE)

cmake_parse_arguments(pref "OPT1" "" "" OPT1)
TEST(pref_OPT1 TRUE)
cmake_parse_arguments(pref "OPT1;OPT2" "" "" OPT1 OPT2)
TEST(pref_OPT1 TRUE)
TEST(pref_OPT2 TRUE)
cmake_parse_arguments(pref "OPT1;OPT2" "" "" "OPT1;OPT2")
TEST(pref_OPT1 TRUE)
TEST(pref_OPT2 TRUE)
cmake_parse_arguments(pref "OPT1;OPT2" "" "")
TEST(pref_OPT1 FALSE)
TEST(pref_OPT2 FALSE)


# single arguments
cmake_parse_arguments(pref "" "SINGLE1" "")
TEST(pref_SINGLE1 UNDEFINED)

cmake_parse_arguments(pref "" "SINGLE1;SINGLE2" "")
TEST(pref_SINGLE1 UNDEFINED)
TEST(pref_SINGLE2 UNDEFINED)


cmake_parse_arguments(pref "" "SINGLE1" "" SINGLE1 foo)
TEST(pref_SINGLE1 foo)
cmake_parse_arguments(pref "" "SINGLE1;SINGLE2" "" SINGLE1 foo SINGLE2 bar)
TEST(pref_SINGLE1 foo)
TEST(pref_SINGLE2 bar)
cmake_parse_arguments(pref "" "SINGLE1;SINGLE2" "" "SINGLE1;foo;SINGLE2;bar")
TEST(pref_SINGLE1 foo)
TEST(pref_SINGLE2 bar)
cmake_parse_arguments(pref "" "SINGLE1;SINGLE2" "")
TEST(pref_SINGLE1 UNDEFINED)
TEST(pref_SINGLE2 UNDEFINED)


# multi arguments

cmake_parse_arguments(pref "" "" "MULTI1")
TEST(pref_MULTI1 UNDEFINED)

cmake_parse_arguments(pref "" "" "MULTI1;MULTI2")
TEST(pref_MULTI1 UNDEFINED)
TEST(pref_MULTI2 UNDEFINED)

cmake_parse_arguments(pref "" "" "MULTI1" MULTI1 foo)
TEST(pref_MULTI1 foo)
cmake_parse_arguments(pref "" "" "MULTI1;MULTI2" MULTI1 foo bar MULTI2 bar foo)
TEST(pref_MULTI1 foo bar)
TEST(pref_MULTI2 bar foo)
cmake_parse_arguments(pref "" "" "MULTI1;MULTI2" "MULTI1;foo;bar;MULTI2;bar;foo")
TEST(pref_MULTI1 foo bar)
TEST(pref_MULTI2 bar foo)
cmake_parse_arguments(pref "" "" "MULTI1;MULTI2")
TEST(pref_MULTI1 UNDEFINED)
TEST(pref_MULTI2 UNDEFINED)
