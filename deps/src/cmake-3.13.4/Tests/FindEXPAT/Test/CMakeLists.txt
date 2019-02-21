cmake_minimum_required(VERSION 3.9)
project(TestFindEXPAT C)
include(CTest)

find_package(EXPAT REQUIRED)

add_definitions(-DCMAKE_EXPECTED_EXPAT_VERSION="${EXPAT_VERSION_STRING}")

add_executable(testexpat_tgt main.c)
target_link_libraries(testexpat_tgt EXPAT::EXPAT)
add_test(NAME testexpat_tgt COMMAND testexpat_tgt)

add_executable(testexpat_var main.c)
target_include_directories(testexpat_var PRIVATE ${EXPAT_INCLUDE_DIRS})
target_link_libraries(testexpat_var PRIVATE ${EXPAT_LIBRARIES})
add_test(NAME testexpat_var COMMAND testexpat_var)
