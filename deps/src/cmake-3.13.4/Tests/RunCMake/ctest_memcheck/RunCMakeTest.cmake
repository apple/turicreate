include(RunCTest)

set(SITE test-site)
set(BUILDNAME test-build)
set(COVERAGE_COMMAND "")

unset(ENV{CTEST_PARALLEL_LEVEL})

function(run_mc_test CASE_NAME CHECKER_COMMAND)
  run_ctest(${CASE_NAME} ${ARGN})
endfunction()

unset(CTEST_EXTRA_CONFIG)
unset(CTEST_EXTRA_CODE)
unset(CTEST_SUFFIX_CODE)
unset(CTEST_MEMCHECK_ARGS)
unset(CMAKELISTS_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add ThreadSanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"\${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testThreadSanitizer.cmake\")
")
run_mc_test(DummyThreadSanitizer "" -DMEMCHECK_TYPE=ThreadSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add standalone LeakSanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1:report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testLeakSanitizer.cmake\")
")
run_mc_test(DummyLeakSanitizer "" -DMEMCHECK_TYPE=LeakSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add AddressSanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1:report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"\${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testAddressSanitizer.cmake\")
")
run_mc_test(DummyAddressSanitizer "" -DMEMCHECK_TYPE=AddressSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add AddressSanitizer/LeakSanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1:report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testAddressLeakSanitizer.cmake\")
")
run_mc_test(DummyAddressLeakSanitizer "" -DMEMCHECK_TYPE=AddressSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add MemorySanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1:report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"\${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testMemorySanitizer.cmake\")
")
run_mc_test(DummyMemorySanitizer "" -DMEMCHECK_TYPE=MemorySanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
# add UndefinedBehaviorSanitizer test
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"\${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testUndefinedBehaviorSanitizer.cmake\")
")
run_mc_test(DummyUndefinedBehaviorSanitizer "" -DMEMCHECK_TYPE=UndefinedBehaviorSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CTEST_EXTRA_CODE "string(REPLACE \" \" \"\\\\ \" PRE_POST_COMMAND \"\${CTEST_MEMORYCHECK_COMMAND}\")

set(CTEST_CUSTOM_PRE_MEMCHECK \"\${PRE_POST_COMMAND} pre command\")
set(CTEST_CUSTOM_POST_MEMCHECK \"\${PRE_POST_COMMAND} post command \")
")
run_mc_test(DummyValgrindPrePost "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CTEST_EXTRA_CODE "set(CTEST_CUSTOM_POST_MEMCHECK \"${MEMCHECK_FAIL}\")")
run_mc_test(DummyValgrindFailPost "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CTEST_EXTRA_CODE "set(CTEST_CUSTOM_PRE_MEMCHECK \"${MEMCHECK_FAIL}\")")
run_mc_test(DummyValgrindFailPre "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CTEST_EXTRA_CONFIG "set(CTEST_CUSTOM_MEMCHECK_IGNORE RunCMakeAgain)\n")
set(CMAKELISTS_EXTRA_CODE "add_test(NAME RunCMakeAgain COMMAND \"\${CMAKE_COMMAND}\" --version)")
run_mc_test(DummyValgrindIgnoreMemcheck "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CONFIG)
unset(CMAKELISTS_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CMAKELISTS_EXTRA_CODE "add_test(NAME RunCMakeAgain COMMAND \"\${CMAKE_COMMAND}\" --version)")
run_mc_test(DummyValgrindTwoTargets "${PSEUDO_VALGRIND}" "-VV")
unset(CMAKELISTS_EXTRA_CODE)

#-----------------------------------------------------------------------------
set(CTEST_EXTRA_CONFIG "set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/does-not-exist\")")
run_mc_test(DummyValgrindInvalidSupFile "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CONFIG)

#-----------------------------------------------------------------------------
# CTest will add the logfile option before any custom options. Set the logfile
# again, this time to an empty string. This will cause the logfile to be
# missing, which will be the prove for us that the custom option is indeed used.
set(CTEST_EXTRA_CONFIG "set(CTEST_MEMORYCHECK_COMMAND_OPTIONS \"--log-file=\")")
run_mc_test(DummyValgrindCustomOptions "${PSEUDO_VALGRIND}")
unset(CTEST_EXTRA_CONFIG)

#-----------------------------------------------------------------------------
run_mc_test(DummyPurify "${PSEUDO_PURIFY}")
run_mc_test(DummyValgrind "${PSEUDO_VALGRIND}")
run_mc_test(DummyBC "${PSEUDO_BC}")
run_mc_test(DummyPurifyNoLogFile "${PSEUDO_PURIFY_NOLOG}")
run_mc_test(DummyValgrindNoLogFile "${PSEUDO_VALGRIND_NOLOG}")
run_mc_test(DummyBCNoLogFile "${PSEUDO_BC_NOLOG}")
run_mc_test(NotExist "\${CTEST_BINARY_DIRECTORY}/no-memcheck-exe")
run_mc_test(Unknown "\${CMAKE_COMMAND}")

#----------------------------------------------------------------------------
set(CTEST_MEMCHECK_ARGS QUIET)
run_mc_test(DummyQuiet "${PSEUDO_VALGRIND}")
unset(CTEST_MEMCHECK_ARGS)

#-----------------------------------------------------------------------------
set(CTEST_SUFFIX_CODE "message(\"Defect count: \${defect_count}\")")
set(CTEST_MEMCHECK_ARGS "DEFECT_COUNT defect_count")
run_mc_test(DummyValgrindNoDefects "${PSEUDO_VALGRIND}")
unset(CTEST_MEMCHECK_ARGS)
unset(CTEST_SUFFIX_CODE)

#-----------------------------------------------------------------------------
set(CTEST_SUFFIX_CODE "message(\"Defect count: \${defect_count}\")")
set(CTEST_MEMCHECK_ARGS "DEFECT_COUNT defect_count")
set(CTEST_EXTRA_CODE
"set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS \"simulate_sanitizer=1:report_bugs=1:history_size=5:exitcode=55\")
")
set(CMAKELISTS_EXTRA_CODE
"add_test(NAME TestSan COMMAND \"${CMAKE_COMMAND}\"
-P \"${RunCMake_SOURCE_DIR}/testLeakSanitizer.cmake\")
")
run_mc_test(DummyLeakSanitizerPrintDefects "" -DMEMCHECK_TYPE=LeakSanitizer)
unset(CMAKELISTS_EXTRA_CODE)
unset(CTEST_EXTRA_CODE)
unset(CTEST_MEMCHECK_ARGS)
unset(CTEST_SUFFIX_CODE)
