macro(math_test expression expected)
    math(EXPR evaluated ${expression} ${ARGN})
    if (NOT evaluated STREQUAL ${expected})
        message(FATAL_ERROR "wrong math result: ${evaluated} != ${expected}")
    endif ()
endmacro()


math_test("100 * 10" 1000)
math_test("100 * 10" 1000 OUTPUT_FORMAT DECIMAL)
math_test("100 * 0xA" 1000 OUTPUT_FORMAT DECIMAL)
math_test("100 * 0xA" 0x3e8 OUTPUT_FORMAT HEXADECIMAL)
