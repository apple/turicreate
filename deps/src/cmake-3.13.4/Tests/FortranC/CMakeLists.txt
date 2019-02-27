cmake_minimum_required(VERSION 2.8)
project(FortranC C Fortran)

# Skip this test for compilers not known to be compatible.
if(NOT (CMAKE_C_COMPILER_ID STREQUAL CMAKE_Fortran_COMPILER_ID OR
        "${CMAKE_C_COMPILER_ID}-${CMAKE_Fortran_COMPILER_ID}" MATCHES "^(MSVC-Intel)$"))
  message(STATUS "${CMAKE_C_COMPILER_ID} C and ${CMAKE_Fortran_COMPILER_ID} Fortran not known to be compatible!")
  return()
endif()

# Wipe out all FortranCInterface information to ensure it re-runs.
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/CMakeFiles/FortranCInterface)

if(FortranC_TEST_FLAGS)
  # Test whether FortranCInterface checks see C flags.
  set(ENV{TEST_OPT_CC} "--test-opt-cc=1")
  set(CMAKE_C_FLAGS "$ENV{TEST_OPT_CC} ${CMAKE_C_FLAGS}")

  # Test whether FortranCInterface checks see Fortran flags.
  set(ENV{TEST_OPT_FC} "--test-opt-fc=1")
  set(CMAKE_Fortran_FLAGS "$ENV{TEST_OPT_FC} ${CMAKE_Fortran_FLAGS}")
endif()

include(FortranCInterface)
FortranCInterface_VERIFY()
