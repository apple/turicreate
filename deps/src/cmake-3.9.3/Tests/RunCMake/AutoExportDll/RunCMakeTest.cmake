include(RunCMake)
set(RunCMake_TEST_NO_CLEAN TRUE)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/AutoExport-build")
# start by cleaning up because we don't clean up along the way
file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
# configure the AutoExport test
run_cmake(AutoExport)
unset(RunCMake_TEST_OPTIONS)
# don't run this test on Watcom or Borland make as it is not supported
if("${RunCMake_GENERATOR}" MATCHES "Watcom WMake|Borland Makefiles")
  return()
endif()
# we build debug so the say.exe will be found in Debug/say.exe for
# Visual Studio generators
if("${RunCMake_GENERATOR}" MATCHES "Visual Studio|Xcode")
  set(INTDIR "Debug/")
endif()
# build AutoExport
run_cmake_command(AutoExportBuild ${CMAKE_COMMAND} --build
  ${RunCMake_TEST_BINARY_DIR} --config Debug --clean-first)
# run the executable that uses symbols from the dll
if(WIN32)
  set(EXE_EXT ".exe")
endif()
run_cmake_command(AutoExportRun
  ${RunCMake_BINARY_DIR}/AutoExport-build/bin/${INTDIR}say${EXE_EXT})
