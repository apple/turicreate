cmake_minimum_required (VERSION 2.6)
project(Assembler C)
message("CTEST_FULL_OUTPUT ")
set(CMAKE_VERBOSE_MAKEFILE 1)

set(SRCS)

# (at least) the following toolchains can process assembler files directly
# and also generate assembler files from C:
if("${CMAKE_GENERATOR}" MATCHES "Makefile|Xcode" AND
    NOT CMAKE_OSX_ARCHITECTURES)
  if((CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang|HP|SunPro|XL)$") OR (CMAKE_C_COMPILER_ID STREQUAL "Intel"  AND  UNIX))
    set(C_FLAGS "${CMAKE_C_FLAGS}")
    separate_arguments(C_FLAGS)
    if(CMAKE_OSX_SYSROOT AND CMAKE_C_SYSROOT_FLAG AND NOT ";${C_FLAGS};" MATCHES ";${CMAKE_C_SYSROOT_FLAG};")
      list(APPEND C_FLAGS ${CMAKE_C_SYSROOT_FLAG} ${CMAKE_OSX_SYSROOT})
    endif()
    # Clang on OS X, and perhaps other compilers, do not support -g
    # for both generating and assembling, so drop it from generating.
    list(REMOVE_ITEM C_FLAGS -g)
    set(SRCS main.s)
    add_custom_command(
      OUTPUT main.s
      COMMAND ${CMAKE_C_COMPILER} ${C_FLAGS} -S ${CMAKE_CURRENT_SOURCE_DIR}/main.c -o main.s
      DEPENDS main.c
      VERBATIM
      )
  endif()
endif()


if(SRCS)
  set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
  enable_language(ASM OPTIONAL)
else()
  message(STATUS "No assembler enabled, using C")
  set(SRCS main.c)
endif()

add_executable(HelloAsm ${SRCS})
