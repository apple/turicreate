include(RunCMake)

set(RunCMake_GENERATOR_PLATFORM "")
run_cmake(NoPlatform)

if("${RunCMake_GENERATOR}" MATCHES "^Visual Studio ([89]|1[01245])( 20[0-9][0-9])?$")
  set(RunCMake_GENERATOR_PLATFORM "x64")
  run_cmake(x64Platform)
else()
  set(RunCMake_GENERATOR_PLATFORM "Bad Platform")
  run_cmake(BadPlatform)
endif()

set(RunCMake_GENERATOR_TOOLSET "")

set(RunCMake_TEST_OPTIONS -A "Extra Platform")
run_cmake(TwoPlatforms)
unset(RunCMake_TEST_OPTIONS)

if("${RunCMake_GENERATOR}" MATCHES "^Visual Studio ([89]|1[01245])( 20[0-9][0-9])?$")
  set(RunCMake_TEST_OPTIONS -DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/TestPlatform-toolchain.cmake)
  run_cmake(TestPlatformToolchain)
  unset(RunCMake_TEST_OPTIONS)
else()
  set(RunCMake_TEST_OPTIONS -DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/BadPlatform-toolchain.cmake)
  run_cmake(BadPlatformToolchain)
  unset(RunCMake_TEST_OPTIONS)
endif()
