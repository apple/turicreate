set(EXTRA_CONFIGURE_FLAGS "")
if(WIN32 AND ${MSYS_MAKEFILES})
  set(EXTRA_CONFIGURE_FLAGS --build=x86_64-w64-mingw32)
endif()
if(APPLE AND TC_BUILD_IOS)
  set(EXTRA_CONFIGURE_FLAGS --host=arm-apple-darwin)
endif()

ExternalProject_Add(ex_node
  DOWNLOAD_COMMAND rsync -a ${CMAKE_SOURCE_DIR}/deps/src/node-v10.15.3/ ${CMAKE_SOURCE_DIR}/deps/build/node-v10.15.3/
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/deps/build/node-v10.15.3/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} "CFLAGS=-fPIC ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" ./configure --prefix=${CMAKE_SOURCE_DIR}/deps/local --openssl-no-asm ${EXTRA_CONFIGURE_FLAGS}
  BUILD_COMMAND make -j4
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/bin/node
  BUILD_IN_SOURCE 1
)

add_executable(node IMPORTED)
set_property(TARGET node PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/bin/node)
add_dependencies(node ex_node)
