cmake_minimum_required (VERSION 3.1)
project(testf C CXX Fortran)

message("CTEST_FULL_OUTPUT ")
set(CMAKE_VERBOSE_MAKEFILE 1)
message("ENV_FLAGS = $ENV{FFLAGS}")
message("CMAKE_Fortran_COMPILER_INIT = ${CMAKE_Fortran_COMPILER_INIT}")
message("CMAKE_Fortran_COMPILER_FULLPATH = ${CMAKE_Fortran_COMPILER_FULLPATH}")
message("CMAKE_Fortran_COMPILER = ${CMAKE_Fortran_COMPILER}")
message("CMAKE_Fortran_FLAGS = ${CMAKE_Fortran_FLAGS}")

set(_SHARED SHARED)
if(CMAKE_Fortran_COMPILER_ID MATCHES "^(XL|VisualAge)$")
  # We do not implement SHARED Fortran libs on AIX yet!
  # Workaround: Set LINKER_LANGUAGE to C, which uses 'xlc' and Fortran implicits.
  set(_SHARED STATIC)
elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  # g77 2.96 does not support shared libs on Itanium because g2c is not -fPIC
  execute_process(COMMAND ${CMAKE_Fortran_COMPILER} --version
                  OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if("${output}" MATCHES "Red Hat .* 2\\.96")
    set(_SHARED STATIC)
  endif()
endif()

# Pick a module .def file with the properly mangled symbol name.
set(world_def "")
if(WIN32 AND NOT CYGWIN)
  if(CMAKE_Fortran_COMPILER_ID MATCHES "GNU")
    set(world_def world_gnu.def)
  elseif(CMAKE_Fortran_COMPILER_ID MATCHES "Intel" OR
      CMAKE_GENERATOR MATCHES "Visual Studio") # Intel plugin
    set(world_def world_icl.def)
  endif()
endif()

add_library(hello STATIC hello.f)
add_library(world ${_SHARED} world.f ${world_def})
add_executable(testf testf.f)
target_link_libraries(testf hello world)

function(test_fortran_c_interface_module)
  message(STATUS "Testing FortranCInterface module")
  # test the C to Fortran interface module
  include(FortranCInterface)
  FortranCInterface_VERIFY()
  FortranCInterface_VERIFY(CXX)
  if(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
    if(NOT CMAKE_Fortran_COMPILER_ID MATCHES "SunPro|MIPSpro|PathScale|Absoft")
      set(module_expected 1)
    endif()
    if(FortranCInterface_MODULE_FOUND OR module_expected)
      set(srcs foo.f)
      set(FORTRAN_FUNCTIONS test_mod:sub)
      set(MYC_DEFS TEST_MOD)
    else()
      message("${CMAKE_Fortran_COMPILER_ID} compilers do not support"
        " linking Fortran module procedures from C")
    endif()
  endif()
  list(APPEND FORTRAN_FUNCTIONS my_sub mysub)
  FortranCInterface_HEADER(foo.h
    MACRO_NAMESPACE "FC_"
    SYMBOL_NAMESPACE "F_"
    SYMBOLS ${FORTRAN_FUNCTIONS}
    )
  include_directories("${CMAKE_CURRENT_BINARY_DIR}")

  # if the name mangling is not found for a F90 compiler
  # print out some diagnostic stuff for the dashboard
  if(NOT FortranCInterface_GLOBAL_FOUND OR
      (NOT FortranCInterface_MODULE_FOUND AND module_expected) )
    find_program(FortranCInterface_EXE
      NAMES FortranCInterface
      PATHS ${FortranCInterface_BINARY_DIR} ${FortranCInterface_BINARY_DIR}/Debug
      NO_DEFAULT_PATH
      )
    find_program(DUMPBIN dumpbin)
    find_program(NM nm)
    if(FortranCInterface_EXE)
      if(DEPENDS)
        execute_process(COMMAND ${DUMPBIN} /symbols "${FortranCInterface_EXE}"
                        OUTPUT_VARIABLE out)
        message("symbols in ${FortranCInterface_EXE}:\n${out}")
      endif()
      if(NM)
        execute_process(COMMAND ${NM} "${FortranCInterface_EXE}"
                        OUTPUT_VARIABLE out)
        message("symbols in ${FortranCInterface_EXE}:\n${out}")
      endif()
    endif()
  endif()
  message("Fortran = ${CMAKE_Fortran_COMPILER_ID}")
  message("C = ${CMAKE_C_COMPILER_ID}")

  add_library(myfort mysub.f ${srcs})

  add_library(myc myc.c)
  target_link_libraries(myc myfort)
  set_property(TARGET myc PROPERTY COMPILE_DEFINITIONS ${MYC_DEFS})

  add_library(mycxx mycxx.cxx)
  target_link_libraries(mycxx myc)

  add_executable(mainc mainc.c)
  target_link_libraries(mainc myc)
  add_executable(maincxx maincxx.c)
  target_link_libraries(maincxx mycxx)

  # print out some stuff to help debug on machines via cdash
  file(READ "${CMAKE_CURRENT_BINARY_DIR}/foo.h" fooh)
  message("foo.h contents:\n${fooh}")
endfunction()

# if the id's match or the compilers are compatible, then
# call the test_fortran_c_interface_module function
if("${CMAKE_Fortran_COMPILER_ID}:${CMAKE_C_COMPILER_ID}" MATCHES
    "(Intel:MSVC|Absoft:GNU)"
    OR ("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "${CMAKE_C_COMPILER_ID}" ))
  test_fortran_c_interface_module()
else()
  message("Fortran does not match c compiler")
  message("Fortran = ${CMAKE_Fortran_COMPILER_ID}")
  message("C = ${CMAKE_C_COMPILER_ID}")
  # hack to make g77 work after CL has been enabled
  # as a languge, cmake needs language specific versions
  # of these variables....
  if(WIN32 AND CMAKE_Fortran_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CREATE_CONSOLE_EXE )
    set(CMAKE_LIBRARY_PATH_FLAG "-L")
    set(CMAKE_LINK_LIBRARY_FLAG "-l")
    set(CMAKE_LINK_LIBRARY_SUFFIX )
  endif()
  # gnu and sunpro do not use the same flags here...
  # however if LDFLAGS is used to set -m64 it causes odd stuf
  # with the fortran build
  if( (CMAKE_C_COMPILER_ID MATCHES "GNU")
      AND (CMAKE_Fortran_COMPILER_ID MATCHES "SunPro"))
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_Fortran_FLAGS "")
  endif()

endif()
