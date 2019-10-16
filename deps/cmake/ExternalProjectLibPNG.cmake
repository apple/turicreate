set(EXTRA_CONFIGURE_FLAGS "")
if(WIN32 AND ${MSYS_MAKEFILES})
  set(EXTRA_CONFIGURE_FLAGS --build=x86_64-w64-mingw32)
endif()
if(APPLE AND TC_BUILD_IOS)
  set(EXTRA_CONFIGURE_FLAGS --host=arm-apple-darwin)
endif()

set(CFLAGS "-fPIC -I${CMAKE_SOURCE_DIR}/deps/local/include -L${CMAKE_SOURCE_DIR}/deps/local/lib")
ExternalProject_Add(ex_libpng
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libpng
  URL ${CMAKE_SOURCE_DIR}/deps/src/libpng-1.6.37/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND env "CFLAGS=${CFLAGS} ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" "CPPFLAGS=${CFLAGS} ${ARCH_FLAG} ${C_REAL_COMPILER_FLAGS}" <SOURCE_DIR>/configure --enable-shared=no --prefix=<INSTALL_DIR> ${EXTRA_CONFIGURE_FLAGS}
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libpng.a
  BUILD_IN_SOURCE 1)

add_dependencies(ex_libpng libza)
add_library(libpnga STATIC IMPORTED)
set_property(TARGET libpnga PROPERTY INTERFACE_LINK_LIBRARIES z)
set_property(TARGET libpnga PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libpng.a)

add_library(libpng INTERFACE )
target_link_libraries(libpng INTERFACE libpnga)
target_compile_definitions(libpng INTERFACE HAS_LIBPNG)
set(HAS_LIBPNG TRUE CACHE BOOL "")
add_dependencies(libpng ex_libpng)
