if(GENERATOR_TYPE STREQUAL "DEB")
  set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
  set(CPACK_DEBIAN_ARCHIVE_TYPE "gnutar")
endif()

set(LONG_FILENAME
  "${CMAKE_CURRENT_BINARY_DIR}/llllllllll_oooooooooo_nnnnnnnnnn_gggggggggg_ffffffffff_iiiiiiiiii_llllllllll_eeeeeeeeee_nnnnnnnnnn_aaaaaaaaaa_mmmmmmmmmm_eeeeeeeeee.txt")

file(WRITE
  "${LONG_FILENAME}"
  "long_filename_test")

install(FILES ${LONG_FILENAME} DESTINATION foo)
