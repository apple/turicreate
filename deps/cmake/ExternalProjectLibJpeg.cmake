set(EXTRA_CONFIGURE_FLAGS "")
if(WIN32 AND ${MSYS_MAKEFILES})
  set(EXTRA_CONFIGURE_FLAGS --build=x86_64-w64-mingw32)
endif()

ExternalProject_Add(ex_libjpeg
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/libjpeg
  URL ${CMAKE_SOURCE_DIR}/deps/src/jpeg-8d/
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-fPIC CPPFLAGS=-fPIC <SOURCE_DIR>/configure --enable-shared=no --prefix=<INSTALL_DIR> ${EXTRA_CONFIGURE_FLAGS}
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libjpeg.a
  BUILD_IN_SOURCE 1)

add_library(libjpega STATIC IMPORTED)
set_property(TARGET libjpega PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/local/lib/libjpeg.a)

add_library(libjpeg INTERFACE )
target_link_libraries(libjpeg INTERFACE libjpega)
target_compile_definitions(libjpeg INTERFACE HAS_LIBJPEG)
set(HAS_LIBJPEG TRUE CACHE BOOL "")
add_dependencies(libjpeg ex_libjpeg)
