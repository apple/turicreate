# create structure required by non root dpkg install
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/root_dir")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir/updates")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir/info")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir/available" "")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir/status" "")

# some programs used by fakeroot require sbin in path so we should not
# leave this to chance (programs: ldconfig and start-stop-daemon)
set(ENV{PATH} "$ENV{PATH}:/usr/local/sbin:/usr/sbin:/sbin")

execute_process(COMMAND ${FAKEROOT_EXECUTABLE} ${DPKG_EXECUTABLE}
      -i --force-not-root --root=${CMAKE_CURRENT_BINARY_DIR}/root_dir
      --admindir=${CMAKE_CURRENT_BINARY_DIR}/root_dir/admindir
      --log=${CMAKE_CURRENT_BINARY_DIR}/root_dir/dpkg.log
      ${FOUND_FILE_1}
    RESULT_VARIABLE install_result_
    ERROR_VARIABLE install_error_
    OUTPUT_QUIET
  )

if(install_result_)
  message(FATAL_ERROR "LONG_FILENAMES package error -  result:"
    " '${install_result_}'; text: '${install_error_}'")
endif()
