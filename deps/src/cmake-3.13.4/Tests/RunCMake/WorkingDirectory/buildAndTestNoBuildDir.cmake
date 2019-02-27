# We want a single test that always passes. We should never actually get to
# configure with this file, so we use a successful configure-build-test
# sequence to denote failure of the test case.
include(CTest)
add_test(NAME willPass
         COMMAND ${CMAKE_COMMAND} -E touch someFile.txt
)
