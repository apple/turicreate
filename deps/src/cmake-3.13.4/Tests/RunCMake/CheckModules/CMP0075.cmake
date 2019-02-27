enable_language(C)
enable_language(CXX)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckIncludeFiles)

set(CMAKE_REQUIRED_LIBRARIES does_not_exist)

#============================================================================

check_include_file("stddef.h" HAVE_STDDEF_H_1)
if(NOT HAVE_STDDEF_H_1)
  message(SEND_ERROR "HAVE_STDDEF_H_1 failed but should have passed.")
endif()
if(NOT _CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_1 did not warn but should have")
endif()
check_include_file("stddef.h" HAVE_STDDEF_H_2) # second does not warn
if(NOT HAVE_STDDEF_H_2)
  message(SEND_ERROR "HAVE_STDDEF_H_2 failed but should have passed.")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_file_cxx("stddef.h" HAVE_STDDEF_H_CXX_1)
if(NOT HAVE_STDDEF_H_CXX_1)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_1 failed but should have passed.")
endif()
if(NOT _CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_1 did not warn but should have")
endif()
check_include_file_cxx("stddef.h" HAVE_STDDEF_H_CXX_2) # second does not warn
if(NOT HAVE_STDDEF_H_CXX_2)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_2 failed but should have passed.")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_files("stddef.h;stdlib.h" HAVE_STDLIB_H_1)
if(NOT HAVE_STDLIB_H_1)
  message(SEND_ERROR "HAVE_STDLIB_H_1 failed but should have passed.")
endif()
if(NOT _CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDLIB_H_1 did not warn but should have")
endif()
check_include_files("stddef.h;stdlib.h" HAVE_STDLIB_H_2) # second does not warn
if(NOT HAVE_STDLIB_H_2)
  message(SEND_ERROR "HAVE_STDLIB_H_2 failed but should have passed.")
endif()
unset(_CIF_CMP0075_WARNED)

#============================================================================
cmake_policy(SET CMP0075 OLD)
# These should not warn.
# These should pass the checks due to ignoring 'does_not_exist'.

check_include_file("stddef.h" HAVE_STDDEF_H_3)
if(NOT HAVE_STDDEF_H_3)
  message(SEND_ERROR "HAVE_STDDEF_H_3 failed but should have passed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_3 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_file_cxx("stddef.h" HAVE_STDDEF_H_CXX_3)
if(NOT HAVE_STDDEF_H_CXX_3)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_3 failed but should have passed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_3 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_files("stddef.h;stdlib.h" HAVE_STDLIB_H_3)
if(NOT HAVE_STDLIB_H_3)
  message(SEND_ERROR "HAVE_STDLIB_H_3 failed but should have passed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDLIB_H_3 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)

#============================================================================
cmake_policy(SET CMP0075 NEW)
# These should not warn.
# These should fail the checks due to requiring 'does_not_exist'.

check_include_file("stddef.h" HAVE_STDDEF_H_4)
if(HAVE_STDDEF_H_4)
  message(SEND_ERROR "HAVE_STDDEF_H_4 passed but should have failed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_4 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_file_cxx("stddef.h" HAVE_STDDEF_H_CXX_4)
if(HAVE_STDDEF_H_CXX_4)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_4 passed but should have failed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDDEF_H_CXX_4 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)

#----------------------------------------------------------------------------

check_include_files("stddef.h;stdlib.h" HAVE_STDLIB_H_4)
if(HAVE_STDLIB_H_4)
  message(SEND_ERROR "HAVE_STDLIB_H_4 passed but should have failed.")
endif()
if(_CIF_CMP0075_WARNED)
  message(SEND_ERROR "HAVE_STDLIB_H_4 warned but should not have")
endif()
unset(_CIF_CMP0075_WARNED)
