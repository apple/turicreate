include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

# specify two keywords for each category and set the first keyword of each
# within ARGN
cmake_parse_arguments(pref "OPT1;OPT2" "SINGLE1;SINGLE2" "MULTI1;MULTI2"
                        OPT1 SINGLE1 foo MULTI1 bar foo bar)
TEST(pref_OPT1 TRUE)
TEST(pref_OPT2 FALSE)
TEST(pref_SINGLE1 foo)
TEST(pref_SINGLE2 UNDEFINED)
TEST(pref_MULTI1 bar foo bar)
TEST(pref_MULTI2 UNDEFINED)
TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)

# same as above but reversed ARGN
cmake_parse_arguments(pref "OPT1;OPT2" "SINGLE1;SINGLE2" "MULTI1;MULTI2"
                        MULTI1 bar foo bar SINGLE1 foo OPT1)
TEST(pref_OPT1 TRUE)
TEST(pref_OPT2 FALSE)
TEST(pref_SINGLE1 foo)
TEST(pref_SINGLE2 UNDEFINED)
TEST(pref_MULTI1 bar foo bar)
TEST(pref_MULTI2 UNDEFINED)
TEST(pref_UNPARSED_ARGUMENTS UNDEFINED)
