project(Turi)

set(ZLIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.11/)

set(ZLIB_SRCS
  ${ZLIB_SRC_DIR}/adler32.c
  ${ZLIB_SRC_DIR}/compress.c
  ${ZLIB_SRC_DIR}/crc32.c
  ${ZLIB_SRC_DIR}/deflate.c
  ${ZLIB_SRC_DIR}/gzclose.c
  ${ZLIB_SRC_DIR}/gzlib.c
  ${ZLIB_SRC_DIR}/gzread.c
  ${ZLIB_SRC_DIR}/gzwrite.c
  ${ZLIB_SRC_DIR}/inflate.c
  ${ZLIB_SRC_DIR}/infback.c
  ${ZLIB_SRC_DIR}/inftrees.c
  ${ZLIB_SRC_DIR}/inffast.c
  ${ZLIB_SRC_DIR}/trees.c
  ${ZLIB_SRC_DIR}/uncompr.c
  ${ZLIB_SRC_DIR}/zutil.c
)

make_library(z SOURCES ${ZLIB_SRCS})

target_include_directories(z SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.11/)

