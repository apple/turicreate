# Always install completion file in local dir
# in order to be sure to always be able to install
# in a local user directory rooted in a single directory.
# packager should either patch that out or
# add symlinks to the files in appropriate places
#  /etc/bash_completion.d/
#  DATADIR/completions (may be /usr/share/<package>/completions
install(FILES cmake cpack ctest DESTINATION ${CMAKE_DATA_DIR}/completions)
