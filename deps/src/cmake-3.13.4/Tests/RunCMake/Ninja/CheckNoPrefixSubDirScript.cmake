# Check that the prefix sub-directory is not repeated.

if(EXISTS "${CUR_BIN_DIR}/${NINJA_OUTPUT_PATH_PREFIX}")
  message(FATAL_ERROR
    "no sub directory named after the CMAKE_NINJA_OUTPUT_PATH_PREFIX "
    "should be in the binary directory."
    )
endif()
