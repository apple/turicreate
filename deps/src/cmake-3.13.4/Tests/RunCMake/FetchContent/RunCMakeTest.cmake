include(RunCMake)

unset(RunCMake_TEST_NO_CLEAN)

run_cmake(MissingDetails)
run_cmake(DirectIgnoresDetails)
run_cmake(FirstDetailsWin)
run_cmake(DownloadTwice)
run_cmake(SameGenerator)
run_cmake(VarDefinitions)
run_cmake(GetProperties)
run_cmake(DirOverrides)

# We need to pass through CMAKE_GENERATOR and CMAKE_MAKE_PROGRAM
# to ensure the test can run on machines where the build tool
# isn't on the PATH. Some build slaves explicitly test with such
# an arrangement (e.g. to test with spaces in the path). We also
# pass through the platform and toolset for completeness, even
# though we don't build anything, just in case this somehow affects
# the way the build tool is invoked.
run_cmake_command(ScriptMode
    ${CMAKE_COMMAND}
    -DCMAKE_GENERATOR=${RunCMake_GENERATOR}
    -DCMAKE_GENERATOR_PLATFORM=${RunCMake_GENERATOR_PLATFORM}
    -DCMAKE_GENERATOR_TOOLSET=${RunCMake_GENERATOR_TOOLSET}
    -DCMAKE_MAKE_PROGRAM=${RunCMake_MAKE_PROGRAM}
    -P ${CMAKE_CURRENT_LIST_DIR}/ScriptMode.cmake
)
