PROJECT(CMZLIB)

# Disable warnings to avoid changing 3rd party code.
if(CMAKE_C_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
elseif(CMAKE_C_COMPILER_ID STREQUAL "PathScale")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -woffall")
endif()

INCLUDE_DIRECTORIES(
  "${CMZLIB_SOURCE_DIR}"
  "${CMZLIB_SOURCE_DIR}/.."
  "${CMZLIB_BINARY_DIR}/.."
  )

# source files for zlib
SET(ZLIB_SRCS
  adler32.c  compress.c  crc32.c  deflate.c  gzio.c   inffast.c
  inflate.c  inftrees.c  trees.c  uncompr.c  zutil.c
  )

# for windows add the .def and .rc files to the source list
# if building shared libs
IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    SET(ZLIB_DLL 1)
    IF(NOT UNIX)
      IF(NOT MINGW)
        SET(ZLIB_SRCS ${ZLIB_SRCS} zlib.def zlib.rc  )
      ENDIF(NOT MINGW)
    ENDIF(NOT UNIX)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/.NoDartCoverage
  ${CMZLIB_BINARY_DIR}/.NoDartCoverage)
CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/zlibDllConfig.h.in
  ${CMZLIB_BINARY_DIR}/zlibDllConfig.h)

FOREACH(name zlib zconf cm_zlib_mangle)
  CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/${name}.h
    ${CMZLIB_BINARY_DIR}/${name}.h COPYONLY)
ENDFOREACH(name)


ADD_LIBRARY(cmzlib ${ZLIB_SRCS})

INSTALL(FILES Copyright.txt DESTINATION ${CMAKE_DOC_DIR}/cmzlib)
