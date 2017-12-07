#################################################################
# Adds a build target called "coverage" for code coverage.
#
# This compiles the code using special GCC flags, run the tests,
# and then generates a nice HTML output. This new "coverage" make
# target will only be available if you build using GCC in Debug
# mode. If any of the required programs (lcov and genhtml) were
# not found, a FATAL_ERROR message is printed.
#
# If not already done, this code will set ENABLE_TEST to ON.
#
# To build the code coverage and open it in your browser do this:
#
#    mkdir debug
#    cd debug
#    cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
#    make -j4
#    make coverage
#    xdg-open coverage/index.html
#################################################################

# Find programs we need 
FIND_PROGRAM(LCOV_EXECUTABLE lcov DOC "Full path to lcov executable")
FIND_PROGRAM(GENHTML_EXECUTABLE genhtml DOC "Full path to genhtml executable")
MARK_AS_ADVANCED(LCOV_EXECUTABLE GENHTML_EXECUTABLE)

# Check, compiler, build types and programs are available
IF(NOT CMAKE_COMPILER_IS_GNUCC)
MESSAGE(FATAL_ERROR "Coverage can only be built on GCC")
ELSEIF(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
MESSAGE(FATAL_ERROR "Coverage can only be built in Debug mode")
ELSEIF(NOT LCOV_EXECUTABLE)
MESSAGE(FATAL_ERROR "lcov executable not found")
ELSEIF(NOT GENHTML_EXECUTABLE)
MESSAGE(FATAL_ERROR "genhtml executable not found")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCC)

# Enable testing if not already done
SET(ENABLE_TEST ON)

#################################################################
# Set special compiler and linker flags for test coverage
#################################################################
# 0. Enable debug: -g
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
# 1. Disable optimizations: -O0
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
# 2. Enable all kind of warnings:
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")
# 3. Enable special coverage flag (HINT: --coverage is a synonym for -fprofile-arcs -ftest-coverage)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
#################################################################

ADD_CUSTOM_TARGET(coverage
COMMAND ${CMAKE_COMMAND} -E echo "Beginning test coverage. Output is written to coverage.log."
COMMAND ${CMAKE_COMMAND} -E echo "COVERAGE-STEP-1/5: Reset all execution counts to zero"
COMMAND ${LCOV_EXECUTABLE} --directory . --zerocounters > coverage.log 2>&1
COMMAND ${CMAKE_COMMAND} -E echo "COVERAGE-STEP-2/5: Run testrunner"
COMMAND ${CMAKE_CTEST_COMMAND} >> coverage.log 2>&1
COMMAND ${CMAKE_COMMAND} -E echo "COVERAGE-STEP-3/5: Collect coverage data"
COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file "./coverage.info" >> coverage.log 2>&1
COMMAND ${CMAKE_COMMAND} -E echo "COVERAGE-STEP-4/5: Generate HTML from coverage data"
COMMAND ${GENHTML_EXECUTABLE} "coverage.info" --title="libarchive-${LIBARCHIVE_VERSION_STRING}" --show-details --legend --output-directory "./coverage"  >> coverage.log 2>&1
COMMAND ${CMAKE_COMMAND} -E echo "COVERAGE-STEP-5/5: Open test coverage HTML output in browser: xdg-open ./coverage/index.html"
COMMENT "Runs testrunner and generates coverage output (formats: .info and .html)")

