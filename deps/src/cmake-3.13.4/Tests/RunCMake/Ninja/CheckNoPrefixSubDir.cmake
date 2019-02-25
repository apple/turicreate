add_custom_target(check_no_prefix_sub_dir ALL
  COMMAND "${CMAKE_COMMAND}"
          "-DNINJA_OUTPUT_PATH_PREFIX=${CMAKE_NINJA_OUTPUT_PATH_PREFIX}"
          "-DCUR_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckNoPrefixSubDirScript.cmake"
  VERBATIM
  )
