file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/abc.txt" "test content")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/user-script.cmake"
  "file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/foo\"
    TYPE FILE FILES \"${CMAKE_CURRENT_BINARY_DIR}/abc.txt\")")
set(CPACK_INSTALL_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/user-script.cmake")

function(run_after_include_cpack)
  file(READ "${CPACK_OUTPUT_CONFIG_FILE}" conf_file_)
  string(REGEX REPLACE "SET\\(CPACK_INSTALL_CMAKE_PROJECTS [^)]*\\)" "" conf_file_ "${conf_file_}")
  file(WRITE "${CPACK_OUTPUT_CONFIG_FILE}" "${conf_file_}")
endfunction()
