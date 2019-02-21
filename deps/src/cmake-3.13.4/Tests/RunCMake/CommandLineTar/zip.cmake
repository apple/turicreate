set(OUTPUT_NAME "test.zip")

set(COMPRESSION_FLAGS cvf)
set(COMPRESSION_OPTIONS --format=zip)

set(DECOMPRESSION_FLAGS xvf)

include(${CMAKE_CURRENT_LIST_DIR}/roundtrip.cmake)

check_magic("504b0304" LIMIT 4 HEX)
