
ExternalProject_Add(ex_libnanomsg
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libnanomsg
  URL ${CMAKE_SOURCE_DIR}/deps/src/nanomsg-1.0.0
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} "CFLAGS=-fPIC ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" "CPPFLAGS=-fPIC ${ARCH_FLAG} ${CPP_REAL_COMPILER_FLAGS}" ${CMAKE_COMMAND} -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=lib -DNN_STATIC_LIB=ON -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} .
  INSTALL_COMMAND make install VERBOSE=1
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libnanomsg.a
  BUILD_IN_SOURCE 1)

add_library(libnanomsga STATIC IMPORTED)
set_property(TARGET libnanomsga PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libnanomsg.a)

add_library(nanomsg INTERFACE )

if (APPLE)
target_link_libraries(nanomsg INTERFACE libnanomsga)
elseif(WIN32)
target_link_libraries(nanomsg INTERFACE libnanomsga ws2_32 mswsock)
else()
target_link_libraries(nanomsg INTERFACE libnanomsga anl)
endif()

add_dependencies(nanomsg ex_libnanomsg)

target_compile_definitions(nanomsg INTERFACE HAS_NANOMSG)
target_compile_definitions(nanomsg INTERFACE NN_STATIC_LIB)

set(HAS_NANOMSG TRUE CACHE BOOL "")
