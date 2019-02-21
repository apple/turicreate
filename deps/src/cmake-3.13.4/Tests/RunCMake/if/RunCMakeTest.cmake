include(RunCMake)

run_cmake(InvalidArgument1)
run_cmake(IsDirectory)
run_cmake(IsDirectoryLong)
run_cmake(duplicate-deep-else)
run_cmake(duplicate-else)
run_cmake(duplicate-else-after-elseif)
run_cmake(elseif-message)
run_cmake(misplaced-elseif)

run_cmake(MatchesSelf)

run_cmake(TestNameThatExists)
run_cmake(TestNameThatDoesNotExist)
