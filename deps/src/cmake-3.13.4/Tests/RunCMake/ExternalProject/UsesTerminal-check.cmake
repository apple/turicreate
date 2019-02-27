cmake_minimum_required(VERSION 3.3)

# If we are using the Ninja generator, we can check and verify that the
# USES_TERMINAL option actually works by examining the Ninja build file.
# This is the only way, since CMake doesn't offer a way to examine the
# options on a custom command after it has been added.  Furthermore,
# there isn't an easy way to test for this by actually running Ninja.
#
# Other generators don't currently support USES_TERMINAL at this time.
# This file can be improved to support them if they do.  Until then, we
# simply assume success for new generator types.
#
# For Ninja, there is a complication.  If the Ninja generator detects a
# version of Ninja < 1.5, it won't actually emit the console pool command,
# because those Ninja versions don't yet support the console pool.  In
# that case, we also have to assume success.

# Check Ninja build output to verify whether or not a target step is in the
# console pool.
macro(CheckNinjaStep _target _step _require)
  if("${_build}" MATCHES
"  DESC = Performing ${_step} step for '${_target}'
  pool = console"
  )
    if(NOT ${_require})
      set(RunCMake_TEST_FAILED "${_target} ${_step} step is in console pool")
      return()
    endif()
  else()
    if(${_require})
      set(RunCMake_TEST_FAILED "${_target} ${_step} step not in console pool")
      return()
    endif()
  endif()
endmacro()

# Check Ninja build output to verify whether each target step is in the
# console pool.
macro(CheckNinjaTarget _target
  _download _update _configure _build _test _install
  )
  CheckNinjaStep(${_target} download ${_download})
  CheckNinjaStep(${_target} update ${_update})
  CheckNinjaStep(${_target} configure ${_configure})
  CheckNinjaStep(${_target} build ${_build})
  CheckNinjaStep(${_target} test ${_test})
  CheckNinjaStep(${_target} install ${_install})
endmacro()

# Load build/make file, depending on generator
if(RunCMake_GENERATOR STREQUAL Ninja)
  # Check the Ninja version.  If < 1.5, console pool isn't supported and
  # so the generator would not emit console pool usage.  That would cause
  # this test to fail.
  execute_process(COMMAND ${RunCMake_MAKE_PROGRAM} --version
    RESULT_VARIABLE _version_result
    OUTPUT_VARIABLE _version
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(_version_result OR _version VERSION_EQUAL "0")
    set(RunCMake_TEST_FAILED "Failed to get Ninja version")
    return()
  endif()
  if(_version VERSION_LESS "1.5")
    return() # console pool not supported on Ninja < 1.5
  endif()

  # Read the Ninja build file
  set(_build_file "${RunCMake_TEST_BINARY_DIR}/build.ninja")

  if(NOT EXISTS "${_build_file}")
    set(RunCMake_TEST_FAILED "Ninja build file not created")
    return()
  endif()

  file(READ "${_build_file}" _build)

  set(_target_check_macro CheckNinjaTarget)
elseif((RunCMake_GENERATOR STREQUAL "") OR NOT DEFINED RunCMake_GENERATOR)
  # protection in case somebody renamed RunCMake_GENERATOR
  set(RunCMake_TEST_FAILED "Unknown generator")
  return()
else()
  # We don't yet know how to test USES_TERMINAL on this generator.
  return()
endif()

# Actual tests:
CheckNinjaTarget(TerminalTest1
  true  true  true  true  true  true )
CheckNinjaTarget(TerminalTest2
  true  false true  false true  false)
CheckNinjaTarget(TerminalTest3
  false true  false true  false true )
CheckNinjaTarget(TerminalTest4
  false false false false false false)
