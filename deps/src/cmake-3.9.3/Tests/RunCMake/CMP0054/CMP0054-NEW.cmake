cmake_policy(SET CMP0054 NEW)

set(FOO "BAR")
set(BAZ "ZZZ")
set(MYTRUE ON)
set(MYNUMBER 3)
set(MYVERSION 3.0)

function(assert_unquoted PREFIX FIRST)
  string(REPLACE ";" " " ARGN_SP "${ARGN}")
  if(${PREFIX} ${FIRST} ${ARGN})
    message(FATAL_ERROR "Assertion failed [${PREFIX} ${FIRST} ${ARGN_SP}]")
  endif()
endfunction()

function(assert_quoted PREFIX FIRST)
  string(REPLACE ";" " " ARGN_SP "${ARGN}")
  if(${PREFIX} "${FIRST}" ${ARGN})
    message(FATAL_ERROR "Assertion failed [${PREFIX} \"${FIRST}\" ${ARGN_SP}]")
  endif()
endfunction()

function(assert FIRST)
  assert_unquoted(NOT ${FIRST} ${ARGN})
  assert_quoted("" ${FIRST} ${ARGN})
endfunction()

assert(MYTRUE)

assert(FOO MATCHES "^BAR$")

assert(MYNUMBER LESS 4)
assert(MYNUMBER GREATER 2)
assert(MYNUMBER EQUAL 3)

assert(FOO STRLESS CCC)
assert(BAZ STRGREATER CCC)
assert(FOO STREQUAL BAR)

assert_unquoted(NOT MYVERSION VERSION_LESS 3.1)
assert_unquoted("" MYVERSION VERSION_LESS 2.9)

assert_quoted(NOT MYVERSION VERSION_LESS 2.9)
assert_quoted(NOT MYVERSION VERSION_LESS 3.1)

assert(MYVERSION VERSION_GREATER 2.9)
assert(MYVERSION VERSION_EQUAL 3.0)
