
# libbz  =================================================================
if (WIN32)
ExternalProject_Add(ex_libz
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libz
  URL ${CMAKE_SOURCE_DIR}/deps/src/zlib-1.2.11/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND
  BUILD_IN_SOURCE 1
  BUILD_COMMAND CC=${CMAKE_C_COMPILER} CFLAGS=-fPIC make -f win32/Makefile.gcc
  INSTALL_COMMAND BINARY_PATH=<INSTALL_DIR>/bin INCLUDE_PATH=<INSTALL_DIR>/include LIBRARY_PATH=<INSTALL_DIR>/lib make -f win32/Makefile.gcc install)
else()
ExternalProject_Add(ex_libz
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libz
  URL ${CMAKE_SOURCE_DIR}/deps/src/zlib-1.2.11/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND env CC=${CMAKE_C_COMPILER} "CFLAGS=-fPIC ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" "CPPFLAGS=-fPIC ${ARCH_FLAG} ${CPP_REAL_COMPILER_FLAGS}" ./configure --static --64 --prefix=<INSTALL_DIR>
  BUILD_IN_SOURCE 1
  BUILD_COMMAND env CC=${CMAKE_C_COMPILER} "CFLAGS=-fPIC ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" "CPPFLAGS=-fPIC ${ARCH_FLAG} ${CPP_REAL_COMPILER_FLAGS}" make install
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libz.a
  INSTALL_COMMAND "" )
endif()
add_library(libza STATIC IMPORTED)
add_dependencies(libza ex_zlib)
set_property(TARGET libza PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libz.a)

add_library(z INTERFACE)
add_dependencies(z ex_libz)
target_link_libraries(z INTERFACE libza)
target_compile_definitions(z INTERFACE HAS_LIBZ)
set(HAS_LIBZ TRUE CACHE BOOL "")
