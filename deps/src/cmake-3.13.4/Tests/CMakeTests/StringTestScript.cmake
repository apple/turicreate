message(STATUS "testname='${testname}'")

if(testname STREQUAL empty) # fail
  string()

elseif(testname STREQUAL bogus) # fail
  string(BOGUS)

elseif(testname STREQUAL random) # pass
  string(RANDOM r)
  message(STATUS "r='${r}'")

elseif(testname STREQUAL toupper_no_variable) # fail
  string(TOUPPER)

elseif(testname STREQUAL ascii_no_variable) # fail
  string(ASCII)

elseif(testname STREQUAL ascii_code_too_small) # fail
  string(ASCII -1 bummer)

elseif(testname STREQUAL ascii_code_too_large) # fail
  string(ASCII 288 bummer)

elseif(testname STREQUAL configure_no_input) # fail
  string(CONFIGURE)

elseif(testname STREQUAL configure_no_variable) # fail
  string(CONFIGURE "this is @testname@")

elseif(testname STREQUAL configure_escape_quotes) # pass
  string(CONFIGURE "this is @testname@" v ESCAPE_QUOTES)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL configure_bogus) # fail
  string(CONFIGURE "this is @testname@" v ESCAPE_QUOTES BOGUS)

elseif(testname STREQUAL regex_no_mode) # fail
  string(REGEX)

elseif(testname STREQUAL regex_match_not_enough_args) # fail
  string(REGEX MATCH)

elseif(testname STREQUAL regex_matchall_not_enough_args) # fail
  string(REGEX MATCHALL)

elseif(testname STREQUAL regex_replace_not_enough_args) # fail
  string(REGEX REPLACE)

elseif(testname STREQUAL regex_bogus_mode) # fail
  string(REGEX BOGUS)

elseif(testname STREQUAL regex_match_multiple_inputs) # pass
  string(REGEX MATCH ".*" v input1 input2 input3 input4)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL regex_match_bad_regex) # fail
  string(REGEX MATCH "(.*" v input)

elseif(testname STREQUAL regex_match_empty_string) # fail
  string(REGEX MATCH "x*" v "")

elseif(testname STREQUAL regex_match_no_match) # pass
  string(REGEX MATCH "xyz" v "abc")
  message(STATUS "v='${v}'")

elseif(testname STREQUAL regex_matchall_multiple_inputs) # pass
  string(REGEX MATCHALL "input" v input1 input2 input3 input4)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL regex_matchall_bad_regex) # fail
  string(REGEX MATCHALL "(.*" v input)

elseif(testname STREQUAL regex_matchall_empty_string) # fail
  string(REGEX MATCHALL "x*" v "")

elseif(testname STREQUAL regex_replace_ends_with_backslash) # fail
  string(REGEX REPLACE "input" "output\\" v input1 input2 input3 input4)

elseif(testname STREQUAL regex_replace_ends_with_escaped_backslash) # pass
  string(REGEX REPLACE "input" "output\\\\" v input1 input2 input3 input4)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL regex_replace_has_linefeed) # pass
  string(REGEX REPLACE "input" "output\\n" v input1 input2 input3 input4)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL regex_replace_has_bogus_escape) # fail
  string(REGEX REPLACE "input" "output\\a" v input1 input2 input3 input4)

elseif(testname STREQUAL regex_replace_bad_regex) # fail
  string(REGEX REPLACE "this (.*" "with that" v input)

elseif(testname STREQUAL regex_replace_empty_string) # fail
  string(REGEX REPLACE "x*" "that" v "")

elseif(testname STREQUAL regex_replace_index_too_small) # fail
  string(REGEX REPLACE "^this (.*)$" "with \\1 \\-1" v "this input")

elseif(testname STREQUAL regex_replace_index_too_large) # fail
  string(REGEX REPLACE "^this (.*)$" "with \\1 \\2" v "this input")

elseif(testname STREQUAL compare_no_mode) # fail
  string(COMPARE)

elseif(testname STREQUAL compare_bogus_mode) # fail
  string(COMPARE BOGUS)

elseif(testname STREQUAL compare_not_enough_args) # fail
  string(COMPARE EQUAL)

elseif(testname STREQUAL replace_not_enough_args) # fail
  string(REPLACE)

elseif(testname STREQUAL replace_multiple_inputs) # pass
  string(REPLACE "input" "output" v input1 input2 input3 input4)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL substring_not_enough_args) # fail
  string(SUBSTRING)

elseif(testname STREQUAL substring_begin_too_large) # fail
  string(SUBSTRING "abcdefg" 25 100 v)

elseif(testname STREQUAL substring_end_larger_than_strlen) # pass
  string(SUBSTRING "abcdefg" 1 100 v)

elseif(testname STREQUAL substring_begin_less_than_zero) # fail
  string(SUBSTRING "abcdefg" -1 4 v)

elseif(testname STREQUAL substring_end_less_than_zero) # pass
  string(SUBSTRING "abcdefg" 0 -1 v)

elseif(testname STREQUAL substring_end_less_than_begin) # pass
  string(SUBSTRING "abcdefg" 6 0 v)

elseif(testname STREQUAL length_not_enough_args) # fail
  string(LENGTH)

elseif(testname STREQUAL strip_not_enough_args) # fail
  string(STRIP)

elseif(testname STREQUAL random_not_enough_args) # fail
  string(RANDOM)

elseif(testname STREQUAL random_3_args) # fail
  string(RANDOM LENGTH 9)

elseif(testname STREQUAL random_5_args) # fail
  string(RANDOM LENGTH 9 ALPHABET "aceimnorsuvwxz")

elseif(testname STREQUAL random_with_length) # pass
  string(RANDOM LENGTH 9 v)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL random_with_alphabet) # pass
  string(RANDOM ALPHABET "aceimnorsuvwxz" v)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL random_bad_length) # fail
  string(RANDOM LENGTH 0 v)

elseif(testname STREQUAL random_empty_alphabet) # pass
  string(RANDOM ALPHABET "" v)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL random_with_length_and_alphabet) # pass
  string(RANDOM LENGTH 9 ALPHABET "aceimnorsuvwxz" v)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL random_with_various_alphabets) # pass
  # small alphabet
  string(RANDOM LENGTH 32 ALPHABET "ACGT" v)
  message(STATUS "v='${v}'")

  # smaller alphabet
  string(RANDOM LENGTH 32 ALPHABET "AB" v)
  message(STATUS "v='${v}'")

  # smallest alphabet
  string(RANDOM LENGTH 32 ALPHABET "Z" v)
  message(STATUS "v='${v}'")

  # smallest length and alphabet
  string(RANDOM LENGTH 1 ALPHABET "Q" v)
  message(STATUS "v='${v}'")

  # seed values -- 2 same, then 1 different
  string(RANDOM LENGTH 32 ALPHABET "ACGT" RANDOM_SEED 987654 v)
  message(STATUS "v='${v}'")
  string(RANDOM LENGTH 32 ALPHABET "ACGT" RANDOM_SEED 987654 v)
  message(STATUS "v='${v}'")
  string(RANDOM LENGTH 32 ALPHABET "ACGT" RANDOM_SEED 876543 v)
  message(STATUS "v='${v}'")

  # alphabet of many colors - use all the crazy keyboard characters
  string(RANDOM LENGTH 78 ALPHABET "~`!@#$%^&*()_-+={}[]\\|:\\;'\",.<>/?" v)
  message(STATUS "v='${v}'")

  message(STATUS "CMAKE_SCRIPT_MODE_FILE='${CMAKE_SCRIPT_MODE_FILE}'")

elseif(testname STREQUAL string_find_with_no_parameter) # fail
  string(FIND)

elseif(testname STREQUAL string_find_with_one_parameter) # fail
  string(FIND "CMake is great.")

elseif(testname STREQUAL string_find_with_two_parameters) # fail
  string(FIND "CMake is great." "a")

elseif(testname STREQUAL string_find_with_three_parameters) # pass
  string(FIND "CMake is great." "a" v)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL string_find_with_four_parameters) # fail
  string(FIND "CMake is great." "a" v v2)

elseif(testname STREQUAL string_find_reverse_with_no_parameter) # fail
  string(FIND REVERSE)

elseif(testname STREQUAL string_find_reverse_with_one_parameter) # fail
  string(FIND "CMake is great." REVERSE)

elseif(testname STREQUAL string_find_reverse_with_two_parameters) # fail
  string(FIND "CMake is great." "a" REVERSE)

elseif(testname STREQUAL string_find_reverse_with_three_parameters) # pass
  string(FIND "CMake is great." "a" v REVERSE)
  message(STATUS "v='${v}'")

elseif(testname STREQUAL string_find_reverse_with_four_parameters_part1) # fail
  string(FIND "CMake is great." "a" v v2 REVERSE)

elseif(testname STREQUAL string_find_reverse_with_four_parameters_part2) # fail
  string(FIND "CMake is great." "a" v REVERSE v2)

elseif(testname STREQUAL string_find_with_no_possible_result) # pass
  string(FIND "CMake is a great application." "z" v)
  message(STATUS "v='${v}'")
  if(NOT(-1 EQUAL ${v}))
    message(SEND_ERROR "FIND sub-command should return -1 but returned ${v}.")
  endif()

elseif(testname STREQUAL string_find_reverse_with_no_possible_result) # pass
  string(FIND "CMake is a great application." "z" v REVERSE)
  message(STATUS "v='${v}'")
  if(NOT(-1 EQUAL ${v}))
    message(SEND_ERROR "FIND REVERSE sub-command should return -1 but returned ${v}.")
  endif()

elseif(testname STREQUAL string_find_with_required_result) # pass
  string(FIND "CMake is a great application." "g" v)
  message(STATUS "v='${v}'")
  if(NOT(11 EQUAL ${v}))
    message(SEND_ERROR "FIND sub-command should return 11 but returned ${v}.")
  endif()

elseif(testname STREQUAL string_find_reverse_with_required_result) # pass
  string(FIND "CMake is a great application." "e" v REVERSE)
  message(STATUS "v='${v}'")
  if(NOT(13 EQUAL ${v}))
    message(SEND_ERROR "FIND REVERSE sub-command should return 13 but returned ${v}.")
  endif()

elseif(testname STREQUAL string_find_word_reverse_with_required_result) # pass
  string(FIND "The command should find REVERSE in this string. Or maybe this REVERSE?!" "REVERSE" v)
  message(STATUS "v='${v}'")
  if(NOT(24 EQUAL ${v}))
    message(SEND_ERROR "FIND sub-command should return 24 but returned ${v}.")
  endif()

elseif(testname STREQUAL string_find_reverse_word_reverse_with_required_result) # pass
  string(FIND "The command should find REVERSE in this string. Or maybe this REVERSE?!" "REVERSE" v REVERSE)
  message(STATUS "v='${v}'")
  if(NOT(62 EQUAL ${v}))
    message(SEND_ERROR "FIND sub-command should return 62 but returned ${v}.")
  endif()

else() # fail
  message(FATAL_ERROR "testname='${testname}' - error: no such test in '${CMAKE_CURRENT_LIST_FILE}'")

endif()
