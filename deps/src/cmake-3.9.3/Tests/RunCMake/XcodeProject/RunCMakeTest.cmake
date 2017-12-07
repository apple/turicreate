include(RunCMake)

run_cmake(XcodeFileType)
run_cmake(XcodeAttributeLocation)
run_cmake(XcodeAttributeGenex)
run_cmake(XcodeAttributeGenexError)
run_cmake(XcodeObjectNeedsEscape)
run_cmake(XcodeObjectNeedsQuote)
run_cmake(XcodeOptimizationFlags)
run_cmake(XcodePreserveNonOptimizationFlags)
run_cmake(XcodePreserveObjcFlag)
if (NOT XCODE_VERSION VERSION_LESS 6)
  run_cmake(XcodePlatformFrameworks)
endif()

run_cmake(PerConfigPerSourceFlags)

# Use a single build tree for a few tests without cleaning.

if(NOT XCODE_VERSION VERSION_LESS 5)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeInstallIOS-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_BINARY_DIR}/ios_install")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeInstallIOS)
  run_cmake_command(XcodeInstallIOS-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)

  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeBundlesOSX-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DTEST_IOS=OFF"
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeBundles)
  run_cmake_command(XcodeBundles-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeBundles-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)

  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeBundlesIOS-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DTEST_IOS=ON"
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeBundles)
  run_cmake_command(XcodeBundles-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeBundles-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(NOT XCODE_VERSION VERSION_LESS 7)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeBundlesWatchOS-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DTEST_WATCHOS=ON"
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeBundles)
  run_cmake_command(XcodeBundles-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeBundles-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(NOT XCODE_VERSION VERSION_LESS 7.1)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeBundlesTvOS-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DTEST_TVOS=ON"
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeBundles)
  run_cmake_command(XcodeBundles-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeBundles-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(NOT XCODE_VERSION VERSION_LESS 7)
  set(RunCMake_TEST_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/osx.cmake")
  run_cmake(XcodeTbdStub)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(NOT XCODE_VERSION VERSION_LESS 6)
  # XcodeIOSInstallCombined
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeIOSInstallCombined-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install"
    "-DCMAKE_IOS_INSTALL_COMBINED=YES")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeIOSInstallCombined)
  run_cmake_command(XcodeIOSInstallCombined-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeIOSInstallCombined-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)

  # XcodeIOSInstallCombinedPrune
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeIOSInstallCombinedPrune-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install"
    "-DCMAKE_IOS_INSTALL_COMBINED=YES")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeIOSInstallCombinedPrune)
  run_cmake_command(XcodeIOSInstallCombinedPrune-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeIOSInstallCombinedPrune-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)

  # XcodeIOSInstallCombinedSingleArch
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeIOSInstallCombinedSingleArch-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_TEST_BINARY_DIR}/_install"
    "-DCMAKE_IOS_INSTALL_COMBINED=YES")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeIOSInstallCombinedSingleArch)
  run_cmake_command(XcodeIOSInstallCombinedSingleArch-build ${CMAKE_COMMAND} --build .)
  run_cmake_command(XcodeIOSInstallCombinedSingleArch-install ${CMAKE_COMMAND} --build . --target install)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(NOT XCODE_VERSION VERSION_LESS 5)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeMultiplatform-build)
  set(RunCMake_TEST_NO_CLEAN 1)

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeMultiplatform)

  # build ios before macos
  run_cmake_command(XcodeMultiplatform-iphonesimulator-build ${CMAKE_COMMAND} --build . -- -sdk iphonesimulator)
  run_cmake_command(XcodeMultiplatform-iphonesimulator-install ${CMAKE_COMMAND} --build . --target install -- -sdk iphonesimulator DESTDIR=${RunCMake_TEST_BINARY_DIR}/_install_iphonesimulator)

  run_cmake_command(XcodeMultiplatform-macosx-build ${CMAKE_COMMAND} --build . -- -sdk macosx)
  run_cmake_command(XcodeMultiplatform-macosx-install ${CMAKE_COMMAND} --build . --target install -- -sdk macosx DESTDIR=${RunCMake_TEST_BINARY_DIR}/_install_macosx)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
  unset(RunCMake_TEST_OPTIONS)
endif()

function(XcodeSchemaGeneration)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/XcodeSchemaGeneration-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS "-DCMAKE_XCODE_GENERATE_SCHEME=ON")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(XcodeSchemaGeneration)
  run_cmake_command(XcodeSchemaGeneration-build xcodebuild -scheme foo build)
endfunction()

if(NOT XCODE_VERSION VERSION_LESS 7)
  XcodeSchemaGeneration()
endif()
