include(RunCMake)

function(run_cmake_toolchain t)
  set(RunCMake_TEST_OPTIONS -DCMAKE_TOOLCHAIN_FILE=${RunCMake_SOURCE_DIR}/${t}-toolchain.cmake)
  run_cmake(${t})
endfunction()

run_cmake_toolchain(CallEnableLanguage)
run_cmake_toolchain(CallProject)
run_cmake_toolchain(FlagsInit)
run_cmake_toolchain(LinkFlagsInit)
