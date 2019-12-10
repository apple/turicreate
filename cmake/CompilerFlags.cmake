include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(CMakeParseArguments)


# check_and_set_compiler_flag
#
# This macro checks optional compiler flags and adds them to the global 
# CMAKE_<LANG>_FLAGS_<MODE> variables as appropriate.  The arguments 
# determine how the flags apply. 
#
# Language (If neither C nor CXX is specified, both are assumed): 
#
# C:   This flag applies to the C compiler.  
# CXX: This flag applies to the C++ compiler.
#
# Mode (If neither DEBUG nor RELEASE are specified, both are assumed):
#
# DEBUG: This flag applies in debug mode.
# RELEASE: This flag applies in release mode.
#
#

macro(check_and_set_compiler_flag FLAG)
  set(options C CXX DEBUG RELEASE RESTRICT_CLANG RESTRICT_GCC)

 CMAKE_PARSE_ARGUMENTS(_acf "${options}" "" "" ${ARGN})

 string(REPLACE "-" "_" flag_name_1 ${FLAG})
 string(REPLACE "=" "_" flag_name ${flag_name_1})

 if(NOT ${_acf_C} AND NOT ${_acf_CXX}) 
   set(_acf_C 1)
   set(_acf_CXX 1)
 endif()

 if(NOT ${_acf_DEBUG} AND NOT ${_acf_RELEASE}) 
   set(_acf_DEBUG 1)
   set(_acf_RELEASE 1)
 endif()

 set(__add_flag 1)

 if(_acf_RESTRICT_CLANG AND NOT ${CLANG})
   message("Skipping Testing of clang specific flag ${FLAG}.")
   set(__add_flag 0)
 endif()
 
 if(_acf_RESTRICT_GCC AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   message("Skipping Testing of gcc specific flag ${FLAG}.")
   set(__add_flag 0)
 endif()
    
 if(${__add_flag}) 

   set(__flag_added "")

   if(${_acf_C})
         
     check_c_compiler_flag(${FLAG} TEST_C_FLAG_${flag_name})
     
     if(${TEST_C_FLAG_${flag_name}})
       if(${_acf_DEBUG} AND ${_acf_RELEASE})
         set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
         set(__flag_added "${__flag_added} C")
       elseif(${_acf_DEBUG})
         set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FLAG}")
         set(__flag_added "${__flag_added} C_DEBUG")
       elseif(${_acf_RELEASE})
         set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${FLAG}")
         set(__flag_added "${__flag_added} C_RELEASE")
       endif()
     endif()
   endif() 

   if(${_acf_CXX})
     check_cxx_compiler_flag(${FLAG} TEST_CXX_FLAG_${flag_name})

     if(${TEST_CXX_FLAG_${flag_name}})
       if(${_acf_DEBUG} AND ${_acf_RELEASE})
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
         set(__flag_added "${__flag_added} CXX")
       elseif(${_acf_DEBUG})
         set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}")
         set(__flag_added "${__flag_added} CXX_DEBUG")
       elseif(${_acf_RELEASE})
         set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FLAG}")
         set(__flag_added "${__flag_added} CXX_RELEASE")
       endif()
     endif()

   endif() 

   message("Added flag ${FLAG} to configurations: ${__flag_added}.")
 endif()

endmacro() 

