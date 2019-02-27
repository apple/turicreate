function (check_test_property test prop)
  get_test_property("${test}" "${prop}" gtp_val)
  get_property(gp_val
    TEST "${test}"
    PROPERTY "${prop}")

  message("get_test_property: -->${gtp_val}<--")
  message("get_property: -->${gp_val}<--")
endfunction ()

include(CTest)
add_test(NAME test COMMAND "${CMAKE_COMMAND}" --help)
set_tests_properties(test PROPERTIES empty "" custom value)

check_test_property(test empty)
check_test_property(test custom)
check_test_property(test noexist)
