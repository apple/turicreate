# a simple C only test case
cmake_minimum_required (VERSION 2.6)
project (MacroTest)

macro(FAILED testname)
  message(SEND_ERROR "${testname} failed ${ARGN}")
endmacro()

macro(PASS testname)
  message("${testname} passed ${ARGN}")
endmacro()

# test ARGC
macro(weird_name)
  if("${ARGC}" EQUAL "3")
    PASS("ARGC")
  else()
    FAILED("ARGC" "Got: ${ARGC}")
  endif()
endmacro()
WeIrD_nAmE(a1 a2 a3)

# test ARGN
macro(test_argn_macro argument)
  if("${ARGN}" EQUAL "3")
    PASS("ARGN")
  else()
    FAILED("ARGN" "Got: ${ARGN}")
  endif()
endmacro()
Test_Argn_Macro(ignored 3)

# case test
macro(strange_macro m)
  set("${m}" strange_macro)
endmacro()
STRANGE_MACRO(var)
set(second_var "second_var")
if("x${var}" STREQUAL "xstrange_macro" AND "x${second_var}" STREQUAL "xsecond_var")
  PASS("Case Test" "(${var} ${second_var})")
else()
  FAILED("Case test" "(${var} ${second_var})")
endif()

# test backing up command
macro(ADD_EXECUTABLE exec)
  _ADD_EXECUTABLE("mini${exec}" ${ARGN})
endmacro()

include(CheckCSourceCompiles)
Check_C_Source_Compiles(
"
#include <stdio.h>
#ifdef __CLASSIC_C__
int main(){
  int ac;
  char*av[];
#else
int main(int ac, char*av[]){
#endif
  if(ac > 1000){return *av[0];}
  return 0;
}"
SOME_CHECK)
if(SOME_CHECK)
  message("CheckCSourceCompiles works")
else()
  message(FATAL_ERROR "CheckCSourceCompiles does not work")
endif()

include(CheckCXXSourceCompiles)
Check_CXX_Source_Compiles(
"
#include <stdio.h>
int main(int ac, char*av[]){
  if(ac > 1000){return *av[0];}
  return 0;
}"
SOME_CHECK)
if(SOME_CHECK)
  message("CheckCXXSourceCompiles works")
else()
  message(FATAL_ERROR "CheckCXXSourceCompiles does not work")
endif()

add_executable(MacroTest macroTest.c)

macro(GET_CURRENT_FILE var)
  set(${var} ${CMAKE_CURRENT_LIST_FILE})
endmacro()
include(context.cmake)
