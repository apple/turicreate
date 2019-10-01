include(RunCTest)

function(run_DisableNotRunTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME DisabledTest COMMAND notACommand --version)
add_test(NAME NotRunTest COMMAND invalidCommand --version)

set_tests_properties(SuccessfulTest PROPERTIES DISABLED false)
set_tests_properties(DisabledTest PROPERTIES DISABLED true)
  ]])
  run_ctest(DisableNotRunTest)
endfunction()
run_DisableNotRunTest()

function(run_DisableFailingTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
set(someFile "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake")
add_test(NAME FailingTest
          COMMAND ${CMAKE_COMMAND} -E compare_files "${someFile}" "${someFile}xxx")
add_test(NAME DisabledFailingTest
          COMMAND ${CMAKE_COMMAND} -E compare_files "${someFile}" "${someFile}xxx")

set_tests_properties(FailingTest PROPERTIES DISABLED false)
set_tests_properties(DisabledFailingTest PROPERTIES DISABLED true)
  ]])
  run_ctest(DisableFailingTest)
endfunction()
run_DisableFailingTest()

function(run_DisableSetupTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME DisabledTest COMMAND "${CMAKE_COMMAND}" --version)
add_test(NAME SuccessfulCleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(DisabledTest PROPERTIES DISABLED true
                                             FIXTURES_SETUP "Foo")
set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_REQUIRED "Foo")
set_tests_properties(SuccessfulCleanupTest PROPERTIES FIXTURES_CLEANUP "Foo")
  ]])
run_ctest(DisableSetupTest)
endfunction()
run_DisableSetupTest()

function(run_DisableRequiredTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME DisabledTest COMMAND "${CMAKE_COMMAND}" --version)
add_test(NAME SuccessfulCleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_SETUP "Foo")
set_tests_properties(DisabledTest PROPERTIES DISABLED true
                                             FIXTURES_REQUIRED "Foo")
set_tests_properties(SuccessfulCleanupTest PROPERTIES FIXTURES_CLEANUP "Foo")
  ]])
run_ctest(DisableRequiredTest)
endfunction()
run_DisableRequiredTest()

function(run_DisableCleanupTest)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME CleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(SuccessfulTest PROPERTIES FIXTURES_REQUIRED "Foo")
set_tests_properties(CleanupTest PROPERTIES DISABLED true
                                            FIXTURES_CLEANUP "Foo")
  ]])
run_ctest(DisableCleanupTest)
endfunction()
run_DisableCleanupTest()

# Consider a fixture that has a setup test, a cleanup test and a disabled test
# which requires that fixture. Limit the test list with a regular expression
# that matches the disabled test but not the setup or cleanup tests, so the
# initial set of tests to be executed contains just the disabled test. Since
# the only test requiring the fixture is disabled, CTest should not
# automatically add in the setup and cleanup tests for the fixture, since no
# enabled test requires them.
function(run_DisableAllTests)
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_test(NAME SetupTest COMMAND "${CMAKE_COMMAND}" --version)
add_test(NAME CleanupTest COMMAND "${CMAKE_COMMAND}" --version)

set_tests_properties(SetupTest PROPERTIES FIXTURES_SETUP "Foo")
set_tests_properties(SuccessfulTest PROPERTIES DISABLED true
                                               FIXTURES_REQUIRED "Foo")
set_tests_properties(CleanupTest PROPERTIES FIXTURES_CLEANUP "Foo")
  ]])
run_ctest(DisableAllTests -R Successful)
endfunction()
run_DisableAllTests()
