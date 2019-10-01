include(RunCMake)

macro(run_cmake test)
  list(APPEND RunCMake_TEST_OPTIONS -DTEST_PROP=${TEST_PROP})
  set(RunCMake-stderr-file ${test}-stderr_${TEST_PROP}.txt)
  _run_cmake(${test})
endmacro()

# Protect tests from running inside the default install prefix.
set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/NotDefaultPrefix")

run_cmake(RelativePathInInterface)
run_cmake(RelativePathInGenex)
run_cmake(export-NOWARN)
run_cmake(SourceDirectoryInInterface)
run_cmake(BinaryDirectoryInInterface)

set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/DirInInstallPrefix/prefix")
run_cmake(DirInInstallPrefix)

configure_file(
  "${RunCMake_SOURCE_DIR}/CMakeLists.txt"
  "${RunCMake_BINARY_DIR}/copy/CMakeLists.txt"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/empty.cpp"
  "${RunCMake_BINARY_DIR}/copy/empty.cpp"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/SourceDirectoryInInterface.cmake"
  "${RunCMake_BINARY_DIR}/copy/SourceDirectoryInInterface.cmake"
  COPYONLY
)
set(RunCMake_TEST_OPTIONS
  "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/copy/SourceDirectoryInInterface/prefix"
  "-DTEST_FILE=${RunCMake_BINARY_DIR}/copy/SourceDirectoryInInterface.cmake"
  )
set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/copy")
run_cmake(InstallInSrcDir)
unset(RunCMake_TEST_SOURCE_DIR)

set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/InstallInBinDir-build/prefix")
set(RunCMake_TEST_OPTIONS
  "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/InstallInBinDir-build/prefix"
  "-DTEST_FILE=${RunCMake_SOURCE_DIR}/BinaryDirectoryInInterface.cmake"
  )
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/InstallInBinDir-build")
run_cmake(InstallInBinDir)
unset(RunCMake_TEST_BINARY_DIR)

configure_file(
  "${RunCMake_SOURCE_DIR}/CMakeLists.txt"
  "${RunCMake_BINARY_DIR}/prefix/src/CMakeLists.txt"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/empty.cpp"
  "${RunCMake_BINARY_DIR}/prefix/src/empty.cpp"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/SourceDirectoryInInterface.cmake"
  "${RunCMake_BINARY_DIR}/prefix/src/SourceDirectoryInInterface.cmake"
  COPYONLY
)

foreach(policyStatus NEW OLD "")
  if (TEST_PROP STREQUAL INCLUDE_DIRECTORIES)
    if (NOT "${policyStatus}" STREQUAL "")
      set(policyOption -DCMAKE_POLICY_DEFAULT_CMP0052=${policyStatus})
    else()
      unset(policyOption)
      set(policyStatus WARN)
    endif()
    set(policySuffix -CMP0052-${policyStatus})
  endif()
  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/prefix" ${policyOption}
    "-DTEST_FILE=${RunCMake_SOURCE_DIR}/BinaryDirectoryInInterface.cmake"
    )
  # Set the RunCMake_TEST_SOURCE_DIR here to the copy too. This is needed to run
  # the test suite in-source properly.  Otherwise the install directory would be
  # a subdirectory or the source directory, which is allowed and tested separately
  # below.
  set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/prefix/src")
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/prefix/BinInInstallPrefix${policySuffix}-build")
  run_cmake(BinInInstallPrefix${policySuffix})
  unset(RunCMake_TEST_BINARY_DIR)

  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/prefix" ${policyOption}
    "-DTEST_FILE=${RunCMake_BINARY_DIR}/prefix/src/SourceDirectoryInInterface.cmake"
    )
  run_cmake(SrcInInstallPrefix${policySuffix})
  unset(RunCMake_TEST_SOURCE_DIR)

  if (NOT TEST_PROP STREQUAL INCLUDE_DIRECTORIES)
    break()
  endif()
endforeach()

set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/InstallPrefixInInterface-build/prefix")
run_cmake(InstallPrefixInInterface)

configure_file(
  "${RunCMake_SOURCE_DIR}/CMakeLists.txt"
  "${RunCMake_BINARY_DIR}/installToSrc/CMakeLists.txt"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/empty.cpp"
  "${RunCMake_BINARY_DIR}/installToSrc/empty.cpp"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/InstallPrefixInInterface.cmake"
  "${RunCMake_BINARY_DIR}/installToSrc/InstallPrefixInInterface.cmake"
  COPYONLY
)
set(RunCMake_TEST_OPTIONS
  "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/installToSrc/InstallPrefixInInterface/prefix"
  "-DTEST_FILE=${RunCMake_BINARY_DIR}/installToSrc/InstallPrefixInInterface.cmake"
  )
set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/installToSrc")
run_cmake(InstallToPrefixInSrcDirOutOfSource)
unset(RunCMake_TEST_SOURCE_DIR)


file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}/installToSrcInSrc")
set(RunCMake_TEST_NO_CLEAN ON)

configure_file(
  "${RunCMake_SOURCE_DIR}/CMakeLists.txt"
  "${RunCMake_BINARY_DIR}/installToSrcInSrc/CMakeLists.txt"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/empty.cpp"
  "${RunCMake_BINARY_DIR}/installToSrcInSrc/empty.cpp"
  COPYONLY
)
configure_file(
  "${RunCMake_SOURCE_DIR}/InstallPrefixInInterface.cmake"
  "${RunCMake_BINARY_DIR}/installToSrcInSrc/InstallPrefixInInterface.cmake"
  COPYONLY
)

set(RunCMake_TEST_OPTIONS
  "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/installToSrcInSrc/InstallPrefixInInterface/prefix"
  "-DTEST_FILE=${RunCMake_BINARY_DIR}/installToSrcInSrc/InstallPrefixInInterface.cmake"
  )
set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/installToSrcInSrc")
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/installToSrcInSrc")
run_cmake(InstallToPrefixInSrcDirInSource)
unset(RunCMake_TEST_SOURCE_DIR)
unset(RunCMake_TEST_BINARY_DIR)
unset(RunCMake_TEST_NO_CLEAN)
