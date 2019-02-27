include(RunCTest)


# 1. Specify subprojects in the CTest script
function(run_CTestScriptVariable)
  set(CTEST_EXTRA_CONFIG "set(CTEST_USE_LAUNCHERS 1)")
  set(CASE_TEST_PREFIX_CODE [[
file(COPY "${CTEST_RUNCMAKE_SOURCE_DIRECTORY}/MyProductionCode"
  DESTINATION ${CTEST_SOURCE_DIRECTORY})
file(COPY "${CTEST_RUNCMAKE_SOURCE_DIRECTORY}/MyExperimentalFeature"
  DESTINATION ${CTEST_SOURCE_DIRECTORY})

set(CTEST_LABELS_FOR_SUBPROJECTS "MyProductionCode;MyExperimentalFeature")
  ]])
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_subdirectory(MyExperimentalFeature)
add_subdirectory(MyProductionCode)
  ]])

  run_ctest(CTestScriptVariable)

  unset(CTEST_EXTRA_CONFIG)
  unset(CASE_TEST_PREFIX_CODE)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()
run_CTestScriptVariable()

# 2. Specify subprojects via a CTest script variable on the command line e.g.
#    ctest -S test.cmake -DCTEST_LABELS_FOR_SUBPROJECTS:STRING="MySubproject"
# Note: This test includes a failing build
function(run_CTestScriptVariableCommandLine)
  set(CTEST_EXTRA_CONFIG "set(CTEST_USE_LAUNCHERS 1)")
  set(CASE_TEST_PREFIX_CODE [[
file(COPY "${CTEST_RUNCMAKE_SOURCE_DIRECTORY}/MyThirdPartyDependency"
  DESTINATION ${CTEST_SOURCE_DIRECTORY})
  ]])
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_subdirectory(MyThirdPartyDependency)
  ]])

  run_ctest(CTestScriptVariableCommandLine "-DCTEST_LABELS_FOR_SUBPROJECTS:STRING=MyThirdPartyDependency")

  unset(CTEST_EXTRA_CONFIG)
  unset(CASE_TEST_PREFIX_CODE)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()
run_CTestScriptVariableCommandLine()

# 3. Set subprojects via a CTest module variable on the command line
#    (will populate DartConfiguration.tcl)
function(run_ModuleVariableCommandLine)
  set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/ModuleVariableCommandLine")
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/ModuleVariableCommandLine-build")
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}")
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  set(CASE_CMAKELISTS_SUFFIX_CODE [[
set(someFile "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake")
add_test(NAME SuccessfulTest COMMAND "${CMAKE_COMMAND}" --version)
set_property(TEST SuccessfulTest PROPERTY LABELS MySubproject)
add_test(NAME FailingTest
          COMMAND ${CMAKE_COMMAND} -E compare_files "${someFile}" "${someFile}xxx")
set_property(TEST FailingTest PROPERTY LABELS MySubproject)
  ]])
  configure_file(${RunCMake_SOURCE_DIR}/CMakeLists.txt.in
                 ${RunCMake_TEST_SOURCE_DIR}/CMakeLists.txt @ONLY)

  set(RunCMake_TEST_OPTIONS "-DCTEST_LABELS_FOR_SUBPROJECTS:STRING=MySubproject")
  run_cmake(ModuleVariableCommandLine-cmake)
  unset(RunCMake_TEST_OPTIONS)
  run_cmake_command(ModuleVariableCommandLine ${CMAKE_CTEST_COMMAND} -C Debug -V)

  unset(RunCMake_TEST_SOURCE_DIR)
  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()
run_ModuleVariableCommandLine()

# 4. Set subprojects via a CTest module variable in CMakeLists.txt
#    (will populate DartConfiguration.tcl)
function(run_ModuleVariableCMakeLists)
  set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/ModuleVariableCMakeLists")
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/ModuleVariableCMakeLists-build")
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}")
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  set(CASE_CMAKELISTS_PREFIX_CODE [[
set(CTEST_LABELS_FOR_SUBPROJECTS MySubproject)
]])

  set(CASE_CMAKELISTS_SUFFIX_CODE [[
set(someFile "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake")
add_test(NAME SuccessfulTest COMMAND "${CMAKE_COMMAND}" --version)
set_property(TEST SuccessfulTest PROPERTY LABELS MySubproject)
add_test(NAME FailingTest
          COMMAND ${CMAKE_COMMAND} -E compare_files "${someFile}" "${someFile}xxx")
set_property(TEST FailingTest PROPERTY LABELS MySubproject)
  ]])
  configure_file(${RunCMake_SOURCE_DIR}/CMakeLists.txt.in
                 ${RunCMake_TEST_SOURCE_DIR}/CMakeLists.txt @ONLY)

  run_cmake(ModuleVariableCMakeLists-cmake)
  run_cmake_command(ModuleVariableCMakeLists ${CMAKE_CTEST_COMMAND} -C Debug -V)

  unset(RunCMake_TEST_SOURCE_DIR)
  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()
run_ModuleVariableCMakeLists()

# The remaining tests set subprojects in CTestConfig.cmake. Settings in this
# config file are shared by both the CTest module and the ctest command line
# `Dashboard Client` mode (e.g. ctest -S).

function(run_ModuleVariableCTestConfig CASE_NAME)
  set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/${CASE_NAME}")
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/${CASE_NAME}-build")
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}")
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  set(CTEST_EXTRA_CONFIG "set(CTEST_LABELS_FOR_SUBPROJECTS \"MySubproject\")")
  configure_file(${RunCMake_SOURCE_DIR}/CTestConfig.cmake.in
                 ${RunCMake_TEST_SOURCE_DIR}/CTestConfig.cmake @ONLY)

  set(CASE_CMAKELISTS_SUFFIX_CODE [[
set(someFile "${CMAKE_CURRENT_SOURCE_DIR}/test.cmake")
add_test(NAME SuccessfulTest COMMAND "${CMAKE_COMMAND}" --version)
set_property(TEST SuccessfulTest PROPERTY LABELS MySubproject)
add_test(NAME FailingTest
          COMMAND ${CMAKE_COMMAND} -E compare_files "${someFile}" "${someFile}xxx")
set_property(TEST FailingTest PROPERTY LABELS MySubproject)
add_test(NAME AnotherSuccessfulTest COMMAND "${CMAKE_COMMAND}" --version)
set_property(TEST AnotherSuccessfulTest PROPERTY LABELS NotASubproject)
  ]])
  configure_file(${RunCMake_SOURCE_DIR}/CMakeLists.txt.in
                 ${RunCMake_TEST_SOURCE_DIR}/CMakeLists.txt @ONLY)

  run_cmake(${CASE_NAME}-cmake)
  run_cmake_command(${CASE_NAME} ${CMAKE_CTEST_COMMAND} -C Debug -V ${ARGN})

  unset(RunCMake_TEST_SOURCE_DIR)
  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(CTEST_EXTRA_CONFIG)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()

# 5. Check that the Subproject timing Summary is printed
run_ModuleVariableCTestConfig(ModuleVariableCTestConfig)

# 6. Use --no-subproject-summary to disable the Subproject timing summary.
run_ModuleVariableCTestConfig(ModuleVariableCTestConfigNoSummary --no-subproject-summary)

# 7. Verify that subprojects are sent to CDash when running a CTest script
function(run_CTestConfigCTestScript)
  set(CTEST_EXTRA_CONFIG [[
set(CTEST_USE_LAUNCHERS 1)
set(CTEST_LABELS_FOR_SUBPROJECTS "MyProductionCode;MyExperimentalFeature")
  ]])
  set(CASE_TEST_PREFIX_CODE [[
file(COPY "${CTEST_RUNCMAKE_SOURCE_DIRECTORY}/MyProductionCode"
  DESTINATION ${CTEST_SOURCE_DIRECTORY})
file(COPY "${CTEST_RUNCMAKE_SOURCE_DIRECTORY}/MyExperimentalFeature"
  DESTINATION ${CTEST_SOURCE_DIRECTORY})
  ]])
  set(CASE_CMAKELISTS_SUFFIX_CODE [[
add_subdirectory(MyExperimentalFeature)
add_subdirectory(MyProductionCode)
  ]])
  run_ctest(CTestConfigCTestScript)

  unset(CTEST_EXTRA_CONFIG)
  unset(CASE_TEST_PREFIX_CODE)
  unset(CASE_CMAKELISTS_SUFFIX_CODE)
endfunction()
run_CTestConfigCTestScript()
