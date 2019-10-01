# This CMakeLists file is *sometimes expected* to result in a configure error.
#
# expect this to succeed:
# ../bin/Release/cmake -G Xcode
#   ../../CMake/Tests/CMakeCommands/build_command
#
# expect this to fail:
# ../bin/Release/cmake -DTEST_ERROR_CONDITIONS:BOOL=ON -G Xcode
#   ../../CMake/Tests/CMakeCommands/build_command
#
# This project exists merely to test the CMake command 'build_command'...
# ...even purposefully calling it with known-bad argument lists to cover
# error handling code.
#

set(cmd "initial")

message("0. begin")

if(TEST_ERROR_CONDITIONS)
  # Test with no arguments (an error):
  build_command()
  message("1. cmd='${cmd}'")

  # Test with unknown arguments (also an error):
  build_command(cmd BOGUS STUFF)
  message("2. cmd='${cmd}'")

  build_command(cmd STUFF BOGUS)
  message("3. cmd='${cmd}'")
else()
  message("(skipping cases 1, 2 and 3 because TEST_ERROR_CONDITIONS is OFF)")
endif()

# Test the one arg signature with none of the optional KEYWORD arguments:
build_command(cmd)
message("4. cmd='${cmd}'")

# Test the two-arg legacy signature:
build_command(legacy_cmd ${CMAKE_MAKE_PROGRAM})
message("5. legacy_cmd='${legacy_cmd}'")
message("   CMAKE_MAKE_PROGRAM='${CMAKE_MAKE_PROGRAM}'")

# Test the optional KEYWORDs:
build_command(cmd CONFIGURATION hoohaaConfig)
message("6. cmd='${cmd}'")

build_command(cmd PROJECT_NAME hoohaaProject)
message("7. cmd='${cmd}'")

build_command(cmd TARGET hoohaaTarget)
message("8. cmd='${cmd}'")

set(cmd "final")
message("9. cmd='${cmd}'")
