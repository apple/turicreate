set(OUTPUT_NAME "test.tar.bz2")

set(COMPRESSION_FLAGS cvjf)
set(COMPRESSION_OPTIONS --format=paxr)

set(DECOMPRESSION_FLAGS xvjf)

include(${CMAKE_CURRENT_LIST_DIR}/roundtrip.cmake)

check_magic("425a68" LIMIT 3 HEX)
