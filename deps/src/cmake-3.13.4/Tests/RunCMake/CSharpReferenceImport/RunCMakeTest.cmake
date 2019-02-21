include(RunCMake)

set(RunCMake_TEST_NO_CLEAN 1)

set(exportFileName "${RunCMake_BINARY_DIR}/module.cmake")
set(exportNameSpace "ex")
set(exportTargetName "ImportLib")

set(RunCMake_TEST_OPTIONS_BASE ${RunCMake_TEST_OPTIONS}
  "-DexportNameSpace:INTERNAL=${exportNameSpace}"
  "-DexportTargetName:INTERNAL=${exportTargetName}")

file(REMOVE "${exportFileName}")

# generate C# & C++ assemblies for use as imported target
set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/ImportLib-build)
file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

set(RunCMake_TEST_OPTIONS ${RunCMake_TEST_OPTIONS_BASE}
  "-DexportFileName:INTERNAL=${exportFileName}"
  # make sure we know the RunCMake_TEST if configuring the project again
  # with cmake-gui for debugging.
  "-DRunCMake_TEST:INTERNAL=ImportLib")

run_cmake(ImportLib)
run_cmake_command(ImportLib-build ${CMAKE_COMMAND} --build . --config Debug)

# generate C# & managed C++ programs which reference imported managed assemblies.
set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/ReferenceImport-build)
file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

set(RunCMake_TEST_OPTIONS ${RunCMake_TEST_OPTIONS_BASE}
  "-DexportFileName:INTERNAL=${exportFileName}"
  # make sure we know the RunCMake_TEST if configuring the project again
  # with cmake-gui for debugging.
  "-DRunCMake_TEST:INTERNAL=ReferenceImport")

run_cmake(ReferenceImport)
run_cmake_command(ReferenceImport-build ${CMAKE_COMMAND} --build . --config Debug)
