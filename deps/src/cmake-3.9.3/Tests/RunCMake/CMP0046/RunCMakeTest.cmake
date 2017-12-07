include(RunCMake)

run_cmake(CMP0046-OLD-missing-dependency)
run_cmake(CMP0046-NEW-missing-dependency)
run_cmake(CMP0046-WARN-missing-dependency)

run_cmake(CMP0046-OLD-existing-dependency)
run_cmake(CMP0046-NEW-existing-dependency)
run_cmake(CMP0046-Duplicate)
