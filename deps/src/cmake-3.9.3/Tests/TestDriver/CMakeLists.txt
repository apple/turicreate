cmake_minimum_required (VERSION 2.6)
project(TestDriverTest)

set(Extra_SRCS testExtraStuff.cxx testExtraStuff2.cxx )
set(Extra_SRCS ${Extra_SRCS};testExtraStuff3.cxx )
include_directories(${TestDriverTest_SOURCE_DIR})
create_test_sourcelist(testSrcs
                       TestDriverTest.cxx
                       test1.cxx
                       test2.cxx
                       subdir/test3.cxx
                       EXTRA_INCLUDE testArgs.h FUNCTION testProccessArgs)

add_executable(TestDriverTest ${testSrcs} ${Extra_SRCS})

