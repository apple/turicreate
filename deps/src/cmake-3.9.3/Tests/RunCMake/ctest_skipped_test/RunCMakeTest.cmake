include(RunCTest)

function(run_SkipTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME SkipTest COMMAND ${skip_command})

set_tests_properties(SkipTest PROPERTIES SKIP_RETURN_CODE 125)
  ]])
  run_ctest(SkipTest)
endfunction()
run_SkipTest()

function(run_SkipSetupTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME SkipTest COMMAND ${skip_command})
add_test(NAME SuccessfulCleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(SkipTest PROPERTIES SKIP_RETURN_CODE 125
                                         FIXTURES_SETUP "Foo")
set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_REQUIRED "Foo")
set_tests_properties(SuccessfulCleanupTest PROPERTIES FIXTURES_CLEANUP "Foo")
  ]])
  run_ctest(SkipSetupTest)
endfunction()
run_SkipSetupTest()

function(run_SkipRequiredTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME SkipTest COMMAND ${skip_command})
add_test(NAME SuccessfulCleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_SETUP "Foo")
set_tests_properties(SkipTest PROPERTIES SKIP_RETURN_CODE 125
                                         FIXTURES_REQUIRED "Foo")
set_tests_properties(SuccessfulCleanupTest PROPERTIES FIXTURES_CLEANUP "Foo")
  ]])
  run_ctest(SkipRequiredTest)
endfunction()
run_SkipRequiredTest()

function(run_SkipCleanupTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME CleanupTest COMMAND ${skip_command})

set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_REQUIRED "Foo")
set_tests_properties(CleanupTest PROPERTIES SKIP_RETURN_CODE 125
                                            FIXTURES_CLEANUP "Foo")
  ]])
  run_ctest(SkipCleanupTest)
endfunction()
run_SkipCleanupTest()
