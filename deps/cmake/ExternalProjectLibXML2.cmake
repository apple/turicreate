
# This library is only needed to parse s3 metadata.
if(NOT ${TC_BUILD_REMOTEFS})
  make_empty_library(libxml2)
  make_empty_library(ex_libxml2)
  return()
endif()

set(EXTRA_CONFIGURE_FLAGS "")
if(WIN32 AND ${MSYS_MAKEFILES})
  set(EXTRA_CONFIGURE_FLAGS --build=x86_64-w64-mingw32)
endif()

if(APPLE)
  set(__SDKCMD "SDKROOT=${CMAKE_OSX_SYSROOT}")
  if(${CMAKE_C_COMPILER_TARGET})
    set(__ARCH_FLAG "--target=${CMAKE_C_COMPILER_TARGET}")
  endif()
endif()


ExternalProject_Add(ex_libxml2
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libxml2
  URL ${CMAKE_SOURCE_DIR}/deps/src/libxml2-2.9.1/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND bash -c "${__SDKCMD} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=\"${__ARCH_FLAG} ${CMAKE_C_FLAGS} -w -Wno-everything\" <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-shared=no --enable-static=yes --without-lzma --libdir=<INSTALL_DIR>/lib --with-python=./ ${EXTRA_CONFIGURE_FLAGS}"
  BUILD_COMMAND cp <SOURCE_DIR>/testchar.c <SOURCE_DIR>/testapi.c && ${__SDKCMD} make
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libxml2.a
  )
# the with-python=./ prevents it from trying to build/install some python stuff
# which is poorly installed (always ways to stick it in a system directory)
include_directories(${CMAKE_SOURCE_DIR}/deps/local/include/libxml2)

add_library(libxml2a STATIC IMPORTED)
set_property(TARGET libxml2a PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libxml2.a)

add_library(libxml2 INTERFACE )
add_dependencies(libxml2 ex_libxml2)
target_link_libraries(libxml2 INTERFACE libxml2a)
if(WIN32)
  target_link_libraries(libxml2 INTERFACE iconv ws2_32)
endif()
target_compile_definitions(libxml2 INTERFACE HAS_LIBXML2)
set(HAS_LIBXML2 TRUE CACHE BOOL "")
