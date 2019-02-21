function(write_empty_file FILENAME)
  file(WRITE "${CMAKE_INSTALL_PREFIX}/${FILENAME}" "")
endfunction()

write_empty_file(empty1.txt)
