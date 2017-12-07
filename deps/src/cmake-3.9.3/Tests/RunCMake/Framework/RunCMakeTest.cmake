include(RunCMake)

function(framework_layout_test Name Toolchain Type)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${Toolchain}${Type}FrameworkLayout-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/${Toolchain}.cmake")
  list(APPEND RunCMake_TEST_OPTIONS "-DFRAMEWORK_TYPE=${Type}")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(FrameworkLayout)
  run_cmake_command(${Name} ${CMAKE_COMMAND} --build .)
endfunction()

framework_layout_test(iOSFrameworkLayout-build ios SHARED)
framework_layout_test(iOSFrameworkLayout-build ios STATIC)
framework_layout_test(OSXFrameworkLayout-build osx SHARED)
framework_layout_test(OSXFrameworkLayout-build osx STATIC)

function(framework_type_test Toolchain Type)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${Toolchain}${Type}FrameworkType-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/${Toolchain}.cmake")
  list(APPEND RunCMake_TEST_OPTIONS "-DFRAMEWORK_TYPE=${Type}")

  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(FrameworkLayout)
  run_cmake_command(FrameworkType${Type}-build ${CMAKE_COMMAND} --build .)
endfunction()

framework_type_test(ios SHARED)
framework_type_test(ios STATIC)
framework_type_test(osx SHARED)
framework_type_test(osx STATIC)
