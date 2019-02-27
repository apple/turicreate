include(RunCTest)

run_ctest(dirNotExist)
run_ctest(buildAndTestNoBuildDir
          --build-and-test
              ${RunCMake_BINARY_DIR}/buildAndTestNoBuildDir
              ${RunCMake_BINARY_DIR}/buildAndTestNoBuildDir/CMakeLists.txt  # Deliberately a file
          --build-generator "${RunCMake_GENERATOR}"
)
