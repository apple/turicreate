include(RunCMake)

# Success tests
run_cmake(realistic)
run_cmake(basic)

# Failure tests
run_cmake(invalid-arg)
run_cmake(bad-version-fuzzy)
run_cmake(bad-version-exact)
