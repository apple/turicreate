message(STATUS "testname='${testname}'")

if(testname STREQUAL empty) # fail
  file()

elseif(testname STREQUAL bogus) # fail
  file(BOGUS ffff)

elseif(testname STREQUAL different_not_enough_args) # fail
  file(DIFFERENT ffff)

elseif(testname STREQUAL download_not_enough_args) # fail
  file(DOWNLOAD ffff)

elseif(testname STREQUAL read_not_enough_args) # fail
  file(READ ffff)

elseif(testname STREQUAL rpath_check_not_enough_args) # fail
  file(RPATH_CHECK ffff)

elseif(testname STREQUAL rpath_remove_not_enough_args) # fail
  file(RPATH_REMOVE ffff)

elseif(testname STREQUAL strings_not_enough_args) # fail
  file(STRINGS ffff)

elseif(testname STREQUAL to_native_path_not_enough_args) # fail
  file(TO_NATIVE_PATH ffff)

elseif(testname STREQUAL read_with_offset) # pass
  file(READ ${CMAKE_CURRENT_LIST_FILE} v OFFSET 42 LIMIT 30)
  message("v='${v}'")

elseif(testname STREQUAL strings_bad_length_minimum) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LENGTH_MINIMUM bogus)

elseif(testname STREQUAL strings_bad_length_maximum) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LENGTH_MAXIMUM bogus)

elseif(testname STREQUAL strings_bad_limit_count) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LIMIT_COUNT bogus)

elseif(testname STREQUAL strings_bad_limit_input) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LIMIT_INPUT bogus)

elseif(testname STREQUAL strings_bad_limit_output) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LIMIT_OUTPUT bogus)

elseif(testname STREQUAL strings_bad_regex) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v REGEX "(")

elseif(testname STREQUAL strings_unknown_arg) # fail
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v BOGUS)

elseif(testname STREQUAL strings_bad_filename) # fail
  file(STRINGS ffff v LIMIT_COUNT 10)

elseif(testname STREQUAL strings_use_limit_count) # pass
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v LIMIT_COUNT 10)
  message("v='${v}'")

elseif(testname STREQUAL strings_use_no_hex_conversion) # pass
  file(STRINGS ${CMAKE_CURRENT_LIST_FILE} v NO_HEX_CONVERSION)
  message("v='${v}'")

elseif(testname STREQUAL glob_recurse_follow_symlinks_no_expression) # fail
  file(GLOB_RECURSE v FOLLOW_SYMLINKS)

elseif(testname STREQUAL glob_recurse_relative_no_directory) # fail
  file(GLOB_RECURSE v RELATIVE)

elseif(testname STREQUAL glob_recurse_relative_no_expression) # fail
  file(GLOB_RECURSE v RELATIVE dddd)

elseif(testname STREQUAL glob_non_full_path) # pass
  file(GLOB_RECURSE v ffff*.*)
  message("v='${v}'")

elseif(testname STREQUAL make_directory_non_full_path) # pass
  file(MAKE_DIRECTORY FileTestScriptDDDD)
  if(NOT EXISTS FileTestScriptDDDD)
    message(FATAL_ERROR "error: non-full-path MAKE_DIRECTORY failed")
  endif()
  file(REMOVE_RECURSE FileTestScriptDDDD)
  if(EXISTS FileTestScriptDDDD)
    message(FATAL_ERROR "error: non-full-path REMOVE_RECURSE failed")
  endif()

elseif(testname STREQUAL different_no_variable) # fail
  file(DIFFERENT FILES)

elseif(testname STREQUAL different_no_files) # fail
  file(DIFFERENT v FILES)

elseif(testname STREQUAL different_unknown_arg) # fail
  file(DIFFERENT v FILES ffffLHS ffffRHS BOGUS)

elseif(testname STREQUAL different_different) # pass
  file(DIFFERENT v FILES ffffLHS ffffRHS)
  message("v='${v}'")

elseif(testname STREQUAL different_same) # pass
  file(DIFFERENT v FILES
    ${CMAKE_CURRENT_LIST_FILE} ${CMAKE_CURRENT_LIST_FILE})
  message("v='${v}'")

elseif(testname STREQUAL rpath_change_unknown_arg) # fail
  file(RPATH_CHANGE BOGUS)

elseif(testname STREQUAL rpath_change_bad_file) # fail
  file(RPATH_CHANGE FILE)

elseif(testname STREQUAL rpath_change_bad_old_rpath) # fail
  file(RPATH_CHANGE FILE ffff OLD_RPATH)

elseif(testname STREQUAL rpath_change_bad_new_rpath) # fail
  file(RPATH_CHANGE FILE ffff OLD_RPATH rrrr NEW_RPATH)

elseif(testname STREQUAL rpath_change_file_does_not_exist) # fail
  file(RPATH_CHANGE FILE ffff OLD_RPATH rrrr NEW_RPATH RRRR)

elseif(testname STREQUAL rpath_change_file_is_not_executable) # fail
  file(RPATH_CHANGE FILE ${CMAKE_CURRENT_LIST_FILE}
    OLD_RPATH rrrr NEW_RPATH RRRR)

elseif(testname STREQUAL rpath_remove_unknown_arg) # fail
  file(RPATH_REMOVE BOGUS)

elseif(testname STREQUAL rpath_remove_bad_file) # fail
  file(RPATH_REMOVE FILE)

elseif(testname STREQUAL rpath_remove_file_does_not_exist) # fail
  file(RPATH_REMOVE FILE ffff)

#elseif(testname STREQUAL rpath_remove_file_is_not_executable) # fail
#  file(RPATH_REMOVE FILE ${CMAKE_CURRENT_LIST_FILE})

elseif(testname STREQUAL rpath_check_unknown_arg) # fail
  file(RPATH_CHECK BOGUS)

elseif(testname STREQUAL rpath_check_bad_file) # fail
  file(RPATH_CHECK FILE)

elseif(testname STREQUAL rpath_check_bad_rpath) # fail
  file(RPATH_CHECK FILE ffff RPATH)

elseif(testname STREQUAL rpath_check_file_does_not_exist) # pass
  file(RPATH_CHECK FILE ffff RPATH rrrr)

elseif(testname STREQUAL rpath_check_file_is_not_executable) # pass
  file(WRITE ffff_rpath_check "")

  if(NOT EXISTS ffff_rpath_check)
    message(FATAL_ERROR "error: non-full-path WRITE failed")
  endif()

  file(RPATH_CHECK FILE ffff_rpath_check RPATH rrrr)
    # careful: if the file does not have the given RPATH, it is deleted...

  if(EXISTS ffff_rpath_check)
    message(FATAL_ERROR "error: non-full-path RPATH_CHECK failed")
  endif()

elseif(testname STREQUAL relative_path_wrong_number_of_args) # fail
  file(RELATIVE_PATH v dir)

elseif(testname STREQUAL relative_path_non_full_path_dir) # fail
  file(RELATIVE_PATH v dir file)

elseif(testname STREQUAL relative_path_non_full_path_file) # fail
  file(RELATIVE_PATH v /dir file)

elseif(testname STREQUAL rename_wrong_number_of_args) # fail
  file(RENAME ffff)

elseif(testname STREQUAL rename_input_file_does_not_exist) # fail
  file(RENAME ffff FFFFGGGG)

elseif(testname STREQUAL to_native_path) # pass
  file(TO_NATIVE_PATH /a/b/c\;/d/e/f:/g/h/i v)
  message("v='${v}'")

elseif(testname STREQUAL download_wrong_number_of_args) # fail
  file(DOWNLOAD zzzz://bogus/ffff)

elseif(testname STREQUAL download_file_with_no_path) # fail
  file(DOWNLOAD zzzz://bogus/ffff ffff)

elseif(testname STREQUAL download_missing_time) # fail
  file(DOWNLOAD zzzz://bogus/ffff ./ffff TIMEOUT)

elseif(testname STREQUAL download_missing_log_var) # fail
  file(DOWNLOAD zzzz://bogus/ffff ./ffff TIMEOUT 2 LOG)

elseif(testname STREQUAL download_missing_status_var) # fail
  file(DOWNLOAD zzzz://bogus/ffff ./ffff TIMEOUT 2 LOG l STATUS)

elseif(testname STREQUAL download_with_bogus_protocol) # pass
  file(DOWNLOAD zzzz://bogus/ffff ./ffff TIMEOUT 2 LOG l STATUS s)
  file(REMOVE ./ffff)
  message("l='${l}'")
  message("s='${s}'")

elseif(testname STREQUAL upload_wrong_number_of_args) # fail
  file(UPLOAD ./ffff)

elseif(testname STREQUAL upload_missing_time) # fail
  file(UPLOAD ./ffff zzzz://bogus/ffff TIMEOUT)

elseif(testname STREQUAL upload_missing_log_var) # fail
  file(UPLOAD ./ffff zzzz://bogus/ffff TIMEOUT 2 LOG)

elseif(testname STREQUAL upload_missing_status_var) # fail
  file(UPLOAD ./ffff zzzz://bogus/ffff TIMEOUT 2 LOG l STATUS)

elseif(testname STREQUAL upload_file_that_doesnt_exist) # fail
  file(UPLOAD ./ffff zzzz://bogus/ffff)

elseif(testname STREQUAL upload_with_bogus_protocol) # pass
  file(UPLOAD ${CMAKE_CURRENT_LIST_FILE} zzzz://bogus/ffff TIMEOUT 2 LOG l STATUS s)
  message("l='${l}'")
  message("s='${s}'")

else() # fail
  message(FATAL_ERROR "testname='${testname}' - error: no such test in '${CMAKE_CURRENT_LIST_FILE}'")

endif()
