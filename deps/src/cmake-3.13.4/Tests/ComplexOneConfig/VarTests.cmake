#
# Test SET
#
set (ZERO_VAR 0)
set (ZERO_VAR_AND_INDENTED 0)
set (ZERO_VAR2 0)

if(ZERO_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED)
else()
  add_definitions(-DSHOULD_BE_DEFINED)
endif()

set(ONE_VAR 1)
set(ONE_VAR_AND_INDENTED 1)
set(ONE_VAR2 1)
set(STRING_VAR "CMake is great" CACHE STRING "test a cache variable")

#
# Test VARIABLE_REQUIRES
#
variable_requires(ONE_VAR
                  ONE_VAR_IS_DEFINED ONE_VAR)

#
# Test various IF/ELSE combinations
#
if(NOT ZERO_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_NOT)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_NOT)
endif()

if(NOT ONE_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_NOT2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_NOT2)
endif()

if(ONE_VAR AND ONE_VAR2)
  add_definitions(-DSHOULD_BE_DEFINED_AND)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_AND)
endif()

if(ONE_VAR AND ZERO_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_AND2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_AND2)
endif()

if(ZERO_VAR OR ONE_VAR2)
  add_definitions(-DSHOULD_BE_DEFINED_OR)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_OR)
endif()

if(ZERO_VAR OR ZERO_VAR2)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_OR2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_OR2)
endif()

if(STRING_VAR MATCHES "^CMake")
  add_definitions(-DSHOULD_BE_DEFINED_MATCHES)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_MATCHES)
endif()

if(STRING_VAR MATCHES "^foo")
  add_definitions(-DSHOULD_NOT_BE_DEFINED_MATCHES2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_MATCHES2)
endif()

if(COMMAND "IF")
  add_definitions(-DSHOULD_BE_DEFINED_COMMAND)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_COMMAND)
endif()

if(COMMAND "ROQUEFORT")
  add_definitions(-DSHOULD_NOT_BE_DEFINED_COMMAND2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_COMMAND2)
endif()

if (EXISTS ${Complex_SOURCE_DIR}/VarTests.cmake)
  add_definitions(-DSHOULD_BE_DEFINED_EXISTS)
else()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_EXISTS)
endif ()

if (EXISTS ${Complex_SOURCE_DIR}/roquefort.txt)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_EXISTS2)
else()
  add_definitions(-DSHOULD_BE_DEFINED_EXISTS2)
endif ()

if (IS_DIRECTORY ${Complex_SOURCE_DIR})
  add_definitions(-DSHOULD_BE_DEFINED_IS_DIRECTORY)
endif ()

if (NOT IS_DIRECTORY ${Complex_SOURCE_DIR}/VarTests.cmake)
  add_definitions(-DSHOULD_BE_DEFINED_IS_DIRECTORY2)
endif ()

set (SNUM1_VAR "1")
set (SNUM2_VAR "2")
set (SNUM3_VAR "1")


if (SNUM1_VAR LESS SNUM2_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_LESS)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_LESS)
endif ()

if (SNUM2_VAR LESS SNUM1_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_LESS2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_LESS2)
endif ()

if (SNUM2_VAR GREATER SNUM1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_GREATER)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_GREATER)
endif ()

if (SNUM1_VAR GREATER SNUM2_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_GREATER2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_GREATER2)
endif ()

if (SNUM2_VAR EQUAL SNUM1_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_EQUAL)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_EQUAL)
endif ()

if (SNUM3_VAR EQUAL SNUM1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_EQUAL)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_EQUAL)
endif ()

if (SNUM1_VAR LESS_EQUAL SNUM2_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_LESS_EQUAL)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_LESS_EQUAL)
endif ()

if (SNUM2_VAR LESS_EQUAL SNUM1_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_LESS_EQUAL2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_LESS_EQUAL2)
endif ()

if (SNUM1_VAR LESS_EQUAL SNUM3_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_LESS_EQUAL3)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_LESS_EQUAL3)
endif ()

if (SNUM2_VAR GREATER_EQUAL SNUM1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_GREATER_EQUAL)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_GREATER_EQUAL)
endif ()

if (SNUM1_VAR GREATER_EQUAL SNUM2_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_GREATER_EQUAL2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_GREATER_EQUAL2)
endif ()

if (SNUM1_VAR GREATER_EQUAL SNUM3_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_GREATER_EQUAL3)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_GREATER_EQUAL3)
endif ()

set (SSTR1_VAR "abc")
set (SSTR2_VAR "bcd")

if (SSTR1_VAR STRLESS SSTR2_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRLESS)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRLESS)
endif ()

if (SSTR2_VAR STRLESS SSTR1_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRLESS2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_STRLESS2)
endif ()

if (SSTR2_VAR STRGREATER SSTR1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRGREATER)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRGREATER)
endif ()

if (SSTR1_VAR STRGREATER SSTR2_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRGREATER2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_STRGREATER2)
endif ()

if (SSTR1_VAR STRLESS_EQUAL SSTR2_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRLESS_EQUAL)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRLESS_EQUAL)
endif ()

if (SSTR2_VAR STRLESS_EQUAL SSTR1_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRLESS_EQUAL2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_STRLESS_EQUAL2)
endif ()

if (SSTR1_VAR STRLESS_EQUAL SSTR1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRLESS_EQUAL3)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRLESS_EQUAL3)
endif ()

if (SSTR2_VAR STRGREATER_EQUAL SSTR1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRGREATER_EQUAL)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRGREATER_EQUAL)
endif ()

if (SSTR1_VAR STRGREATER_EQUAL SSTR2_VAR)
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRGREATER_EQUAL2)
else ()
  add_definitions(-DSHOULD_BE_DEFINED_STRGREATER_EQUAL2)
endif ()

if (SSTR1_VAR STRGREATER_EQUAL SSTR1_VAR)
  add_definitions(-DSHOULD_BE_DEFINED_STRGREATER_EQUAL3)
else ()
  add_definitions(-DSHOULD_NOT_BE_DEFINED_STRGREATER_EQUAL3)
endif ()

#
# Test FOREACH
#
foreach (INDEX 1 2)
  set(FOREACH_VAR${INDEX} "VALUE${INDEX}")
endforeach()

set(FOREACH_CONCAT "")
foreach (INDEX a;b;c;d;e;f;g)
  string(APPEND FOREACH_CONCAT "${INDEX}")
endforeach()
