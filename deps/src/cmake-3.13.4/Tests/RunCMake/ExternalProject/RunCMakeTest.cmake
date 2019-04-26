include(RunCMake)

run_cmake(IncludeScope-Add)
run_cmake(IncludeScope-Add_Step)
run_cmake(NoOptions)
run_cmake(SourceEmpty)
run_cmake(SourceMissing)
run_cmake(CMAKE_CACHE_ARGS)
run_cmake(CMAKE_CACHE_DEFAULT_ARGS)
run_cmake(CMAKE_CACHE_mix)
run_cmake(NO_DEPENDS)
run_cmake(Add_StepDependencies)
run_cmake(Add_StepDependencies_iface)
run_cmake(Add_StepDependencies_iface_step)
run_cmake(Add_StepDependencies_no_target)
run_cmake(UsesTerminal)

# Run both cmake and build steps. We always do a clean before the
# build to ensure that the download step re-runs each time.
function(__ep_test_with_build testName)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${testName}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${testName})
  run_cmake_command(${testName}-clean ${CMAKE_COMMAND} --build . --target clean)
  run_cmake_command(${testName}-build ${CMAKE_COMMAND} --build .)
endfunction()

__ep_test_with_build(MultiCommand)

# We can't test the substitution when using the old MSYS due to
# make/sh mangling the paths (substitution is performed correctly,
# but the mangling means we can't reliably test the output).
# There is no such issue when using the newer MSYS though. Therefore,
# we need to bypass the substitution test if using old MSYS.
# See merge request 1537 for discussion.
set(doSubstitutionTest YES)
if(RunCMake_GENERATOR STREQUAL "MSYS Makefiles")
  execute_process(COMMAND uname OUTPUT_VARIABLE uname)
  if(uname MATCHES "^MINGW32_NT")
      set(doSubstitutionTest NO)
  endif()
endif()
if(doSubstitutionTest)
    __ep_test_with_build(Substitutions)
endif()
