include(RunCMake)

# Protect tests from running inside the default install prefix.
set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/NotDefaultPrefix")

run_cmake(CMP0041-OLD)
run_cmake(CMP0041-NEW)
run_cmake(CMP0041-WARN)
run_cmake(CMP0041-tid-OLD)
run_cmake(CMP0041-tid-NEW)
run_cmake(CMP0041-tid-WARN)
