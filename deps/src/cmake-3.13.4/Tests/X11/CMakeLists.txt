# a simple C only test case
cmake_minimum_required (VERSION 2.6)
project (UseX11 CXX C)

include (${CMAKE_ROOT}/Modules/FindX11.cmake)
message("X11_FOUND: ${X11_FOUND}")

add_executable (UseX11 X11.c)
install(TARGETS UseX11 DESTINATION bin)

# so for universal binaries this test will fail if
#
if(APPLE)
  list(LENGTH CMAKE_OSX_ARCHITECTURES NUMARCH)
  if(NUMARCH GREATER 1)
    if(NOT EXISTS /usr/X11R6/lib//libSM.6.dylib)
      set(X11_FOUND FALSE)
      message("disable X11, because of universal binary and sysroot")
    endif()
  endif()
endif()

if(X11_FOUND)
  add_definitions(-DCMAKE_HAS_X)
  include_directories(${X11_INCLUDE_DIR})
  target_link_libraries(UseX11 ${X11_LIBRARIES})
  if(APPLE)
    add_executable(HelloWorldX11 HelloWorldX11.cxx)
    target_link_libraries(HelloWorldX11 ${X11_LIBRARIES})
    install(TARGETS HelloWorldX11 DESTINATION bin)

    set(CPACK_BINARY_OSXX11 ON CACHE BOOL "" FORCE)
    set(CPACK_BINARY_PACKAGEMAKER OFF CACHE BOOL "" FORCE )
    set(CPACK_PACKAGE_NAME HelloWorldX11Package)
    set(CPACK_PACKAGE_EXECUTABLES HelloWorldX11 HelloWorldX11)
  endif()
endif()

# build a CPack driven installer package
include(CPack)
