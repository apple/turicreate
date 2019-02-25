cmake_minimum_required(VERSION 2.8)
project(CheckCompilerRelatedVariables)


function(echo_var var)
  if(DEFINED ${var})
    message("${var}='${${var}}' is defined")
  else()
    message("${var}='${${var}}' is NOT defined")
  endif()
endfunction()


#
# Check that the correct number of MSVC** variables are defined...
#
set(msvc_total 0)

if(DEFINED MSVC60)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC70)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC71)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC80)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC90)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC10)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC11)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC12)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()
if(DEFINED MSVC14)
  math(EXPR msvc_total "${msvc_total} + 1")
endif()

echo_var(MSVC)
echo_var(MSVC60)
echo_var(MSVC70)
echo_var(MSVC71)
echo_var(MSVC80)
echo_var(MSVC90)
echo_var(MSVC10)
echo_var(MSVC11)
echo_var(MSVC12)
echo_var(MSVC14)
echo_var(MSVC_IDE)

if(MSVC)
  #
  # MSVC is set in cl.cmake when cl is the compiler...
  #
  # Exactly one of the numbered variables should also be set
  # indicating which version of the cl compiler / Visual Studio
  # is in use...
  #
  if(msvc_total EQUAL 1)
    message("test passes: exactly one MSVC** variable is defined...")
  else()
    message(FATAL_ERROR "error: ${msvc_total} MSVC** variables are defined -- exactly 1 expected")
  endif()
  if(NOT DEFINED MSVC_IDE)
    message(FATAL_ERROR "MSVC_IDE not defined but should be!")
  elseif("${CMAKE_GENERATOR}" MATCHES "Visual Studio" AND NOT MSVC_IDE)
    message(FATAL_ERROR "MSVC_IDE is not true but should be (${CMAKE_GENERATOR})!")
  elseif(NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio" AND MSVC_IDE)
    message(FATAL_ERROR "MSVC_IDE is true but should not be (${CMAKE_GENERATOR})!")
  endif()
else()
  #
  # The compiler is something other than cl... None of the MSVC** variables
  # should be defined...
  #
  if(msvc_total EQUAL 0)
    message("test passes: no MSVC** variables are defined on non-MSVC build...")
  else()
    message(FATAL_ERROR "error: ${msvc_total} MSVC** variables are defined -- exactly 0 expected")
  endif()
  if(DEFINED MSVC_IDE)
    message(FATAL_ERROR "MSVC_IDE is defined but should not be!")
  endif()
endif()


#
# This is a no-op executable... If this test is going to fail, it fails during
# the configure step while cmake is configuring this CMakeLists.txt file...
#

file(WRITE
  "${CMAKE_CURRENT_BINARY_DIR}/main.cxx"
  "int main() { return 0; }
"
  )

add_executable(
  CheckCompilerRelatedVariables
  "${CMAKE_CURRENT_BINARY_DIR}/main.cxx"
  )
