set(OUTPUT_NAME "test.tar")

set(COMPRESSION_FLAGS cvf)
set(COMPRESSION_OPTIONS --format=gnutar)

set(DECOMPRESSION_FLAGS xvf)

include(${CMAKE_CURRENT_LIST_DIR}/roundtrip.cmake)

check_magic("7573746172202000" OFFSET 257 LIMIT 8 HEX)
