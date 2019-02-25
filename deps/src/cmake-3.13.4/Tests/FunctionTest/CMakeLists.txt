# a simple C only test case
cmake_minimum_required (VERSION 2.6)
project (FunctionTest)

function(FAILED testname)
  message(SEND_ERROR "${testname} failed ${ARGN}")
endfunction()

function(PASS testname)
  message("${testname} passed ${ARGN}")
endfunction()


# test scope
set(COUNT 3)
function(scope_test)
  set(COUNT 4)
endfunction()
scope_test()
if(COUNT EQUAL "3")
  PASS("scope")
else()
  FAILED("COUNT Got: ${COUNT}")
endif()

# test ARGC
function(weird_name)
  if("${ARGC}" EQUAL "3")
    PASS("ARGC")
  else()
    FAILED("ARGC" "Got: ${ARGC}")
  endif()
endfunction()
WeIrD_nAmE(a1 a2 a3)

# test ARGN
function(test_argn_function argument)
  if("${ARGN}" EQUAL "3")
    PASS("ARGN")
  else()
    FAILED("ARGN" "Got: ${ARGN}")
  endif()
endfunction()
Test_Argn_Function(ignored 3)

# test argument naming and raise scope
function(track_find_variable cache_variable is_changed)
 set("${is_changed}" changed PARENT_SCOPE)
endfunction()
track_find_variable(testvar is_changed)
if ("${is_changed}" STREQUAL changed)
  pass("same argument name test")
else ()
  pass("same argument name test")
endif ()

include("Util.cmake")
tester()
if (tester_res STREQUAL "${CMAKE_CURRENT_LIST_FILE}")
  pass("CMAKE_CURRENT_LIST_FILE test")
else ()
  pass("CMAKE_CURRENT_LIST_FILE test")
endif ()



# test recursion and return via set(... PARENT_SCOPE)
function (factorial argument result)
  if (argument LESS 2)
    set (lresult 1)
  else ()
    math (EXPR temp "${argument} - 1")
    factorial (${temp} tresult)
    math (EXPR lresult "${argument}*${tresult}")
  endif ()
  set ("${result}" "${lresult}" PARENT_SCOPE)
endfunction ()

factorial (5 fresult)
if (fresult EQUAL 120)
  pass("factorial")
else ()
  failed ("factorial, computed ${fresult} instead of 120")
endif ()



# case test
function(strange_function m)
  set("${m}" strange_function PARENT_SCOPE)
endfunction()

STRANGE_FUNCTION(var)
set(second_var "second_var")
if("x${var}" STREQUAL "xstrange_function" AND "x${second_var}" STREQUAL "xsecond_var")
  PASS("Case Test" "(${var} ${second_var})")
else()
  FAILED("Case test" "(${var} ${second_var})")
endif()

# test backing up command
function(ADD_EXECUTABLE exec)
  _ADD_EXECUTABLE(mini${exec} ${ARGN})
endfunction()

# var undef case
function(undef_var m)
  set("${m}" PARENT_SCOPE)
endfunction()

set(FUNCTION_UNDEFINED 1)
undef_var(FUNCTION_UNDEFINED)
if(DEFINED FUNCTION_UNDEFINED)
  FAILED("Function Undefine Test" "(${FUNCTION_UNDEFINED})")
else()
  PASS("Function Undefine Test" "(${FUNCTION_UNDEFINED})")
endif()

# Subdirectory scope raise.
set(SUBDIR_UNDEFINED 1)
add_subdirectory(SubDirScope)
if(DEFINED SUBDIR_UNDEFINED)
  FAILED("Subdir Undefine Test" "(${SUBDIR_UNDEFINED})")
else()
  PASS("Subdir Undefine Test" "(${SUBDIR_UNDEFINED})")
endif()
if(DEFINED SUBDIR_DEFINED)
  PASS("Subdir Define Test" "(${SUBDIR_DEFINED})")
else()
  FAILED("Subdir Define Test" "(${SUBDIR_DEFINED})")
endif()

# Test function-scoped directory.
function(ADD_SUBDIR2 dir)
  add_subdirectory("${dir}" "${dir}2")
  # The parent scope sets in the subdir should be visible here.
  if(DEFINED SUBDIR_UNDEFINED)
    FAILED("Subdir Function Undefine Test 1" "(${SUBDIR_UNDEFINED})")
  else()
    PASS("Subdir Function Undefine Test 1" "(${SUBDIR_UNDEFINED})")
  endif()
  if(DEFINED SUBDIR_DEFINED)
    PASS("Subdir Function Define Test 1" "(${SUBDIR_DEFINED})")
  else()
    FAILED("Subdir Function Define Test 1" "(${SUBDIR_DEFINED})")
  endif()
endfunction()

# Reset test variables.
set(SUBDIR_UNDEFINED 1)
set(SUBDIR_DEFINED)

# Run test function.
ADD_SUBDIR2(SubDirScope)

# The parent scope sets in the subdir should not be visible here.
if(DEFINED SUBDIR_UNDEFINED)
  PASS("Subdir Function Undefine Test 2" "(${SUBDIR_UNDEFINED})")
else()
  FAILED("Subdir Function Undefine Test 2" "(${SUBDIR_UNDEFINED})")
endif()
if(DEFINED SUBDIR_DEFINED)
  FAILED("Subdir Function Define Test 2" "(${SUBDIR_DEFINED})")
else()
  PASS("Subdir Function Define Test 2" "(${SUBDIR_DEFINED})")
endif()

add_executable(FunctionTest functionTest.c)

# Use the PROJECT_LABEL property: in IDEs, the project label should appear
# in the UI rather than the target name. If this were a good test of the
# property rather than just a smoke test, it would verify that the label
# actually appears in the UI of the IDE... Or at least that the text appears
# somewhere in the generated project files.
set_property(TARGET miniFunctionTest
  PROPERTY PROJECT_LABEL "Test de Fonctionnement")
