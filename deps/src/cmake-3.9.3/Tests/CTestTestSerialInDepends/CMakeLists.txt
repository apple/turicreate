cmake_minimum_required(VERSION 2.8.12)

project(CTestTestSerialInDepends)

enable_testing()

function(my_add_test NAME COST)
    add_test(NAME ${NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} -DTEST_NAME=${NAME}
        -S ${CMAKE_CURRENT_SOURCE_DIR}/test.ctest)
    set_tests_properties(${NAME} PROPERTIES COST ${COST})
endfunction()

my_add_test(i_like_company 1000)
my_add_test(i_like_company_too 0)

my_add_test(i_have_dependencies 1000)
set_tests_properties(i_have_dependencies PROPERTIES
    DEPENDS "i_want_to_be_alone")

my_add_test(i_want_to_be_alone 100)
set_tests_properties(i_want_to_be_alone PROPERTIES RUN_SERIAL 1)
