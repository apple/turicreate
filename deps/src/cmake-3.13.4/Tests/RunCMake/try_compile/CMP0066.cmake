enable_language(C)
set(CMAKE_C_FLAGS_RELEASE "-DPP_ERROR ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

#-----------------------------------------------------------------------------
message("before try_compile with CMP0066 WARN-default")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
string(REPLACE "\n" "\n  " out "  ${out}")
if(NOT RESULT)
  message(FATAL_ERROR "try_compile with CMP0066 WARN-default failed but should have passed:\n${out}")
else()
  message(STATUS "try_compile with CMP0066 WARN-default worked as expected")
endif()
message("after try_compile with CMP0066 WARN-default")

#-----------------------------------------------------------------------------
set(CMAKE_POLICY_WARNING_CMP0066 ON)
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
string(REPLACE "\n" "\n  " out "  ${out}")
if(NOT RESULT)
  message(FATAL_ERROR "try_compile with CMP0066 WARN-enabled failed but should have passed:\n${out}")
else()
  message(STATUS "try_compile with CMP0066 WARN-enabled worked as expected")
endif()

#-----------------------------------------------------------------------------
cmake_policy(SET CMP0066 OLD)
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
string(REPLACE "\n" "\n  " out "  ${out}")
if(NOT RESULT)
  message(FATAL_ERROR "try_compile with CMP0066 OLD failed but should have passed:\n${out}")
else()
  message(STATUS "try_compile with CMP0066 OLD worked as expected")
endif()

#-----------------------------------------------------------------------------
cmake_policy(SET CMP0066 NEW)
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  OUTPUT_VARIABLE out
  )
string(REPLACE "\n" "\n  " out "  ${out}")
if(RESULT)
  message(FATAL_ERROR "try_compile with CMP0066 NEW passed but should have failed:\n${out}")
elseif(NOT "x${out}" MATCHES "PP_ERROR is defined")
  message(FATAL_ERROR "try_compile with CMP0066 NEW did not fail with PP_ERROR:\n${out}")
else()
  message(STATUS "try_compile with CMP0066 NEW worked as expected")
endif()
