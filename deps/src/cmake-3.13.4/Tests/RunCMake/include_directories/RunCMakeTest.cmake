include(RunCMake)

# Protect tests from running inside the default install prefix.
set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/NotDefaultPrefix")

run_cmake(NotFoundContent)
run_cmake(DebugIncludes)
run_cmake(DirectoryBefore)
run_cmake(TID-bad-target)
run_cmake(ImportedTarget)
run_cmake(CMP0021)
run_cmake(install_config)
run_cmake(incomplete-genex)
