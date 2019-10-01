set(OUTPUT_NAME "test.tar.gz")

set(COMPRESSION_FLAGS cvzf)
set(COMPRESSION_OPTIONS --format=gnutar)

set(DECOMPRESSION_FLAGS xvzf)

include(${CMAKE_CURRENT_LIST_DIR}/roundtrip.cmake)

check_magic("1f8b" LIMIT 2 HEX)
