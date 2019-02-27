cmake_minimum_required(VERSION 2.8)

project(ArgumentExpansion)

function (argument_tester expected expected_len)
    list(LENGTH ARGN argn_len)
    list(LENGTH ${expected} expected_received_len)

    if (NOT ${expected_received_len} EQUAL ${expected_len})
        message(STATUS "Unexpected: Expanding expected values isn't working")
    endif ()

    if (${argn_len} EQUAL ${expected_len})
        set(i 0)
        while (i LESS ${argn_len})
            list(GET ARGN ${i} argn_value)
            list(GET ${expected} ${i} expected_value)

            if (NOT "${argn_value}" STREQUAL "${expected_value}")
                message(STATUS "Unexpected: Argument ${i} doesn't match")
                message(STATUS "    Expected: ${expected_value}")
                message(STATUS "    Received: ${argn_value}")
            endif ()

            math(EXPR i "${i} + 1")
        endwhile ()
    else ()
        message(STATUS "Unexpected: Lengths of arguments don't match")
        message(STATUS "    Expected: ${expected_len}")
        message(STATUS "    Received: ${argn_len}")
    endif ()
endfunction ()

set(empty_test)
message(STATUS "Test: Empty arguments")
argument_tester(empty_test 0 ${empty_test})

set(single_arg_test
    "single arg")
message(STATUS "Test: Single argument")
argument_tester(single_arg_test 1 ${single_arg_test})

set(multiple_arg_test
    "first arg"
    "second arg")
message(STATUS "Test: Multiple arguments")
argument_tester(multiple_arg_test 2 ${multiple_arg_test})

set(nested_list_arg_test
    "${multiple_arg_test}"
    "first arg"
    "second arg")
message(STATUS "Test: Nested list argument flattens")
argument_tester(nested_list_arg_test 4 ${nested_list_arg_test})

set(semicolon_arg_test
    "pre\;post")
set(semicolon_arg_test_flat "pre;post")
message(STATUS "Test: Semicolon argument flattens")
argument_tester(semicolon_arg_test_flat 2 ${semicolon_arg_test})
