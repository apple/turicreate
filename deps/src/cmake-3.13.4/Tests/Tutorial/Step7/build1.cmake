set(CTEST_SOURCE_DIRECTORY "$ENV{HOME}/Dashboards/My Tests/CMake/Tests/Tutorial/Step7")
set(CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}-build1")

set(CTEST_CMAKE_COMMAND "cmake")
set(CTEST_COMMAND "ctest -D Experimental")
