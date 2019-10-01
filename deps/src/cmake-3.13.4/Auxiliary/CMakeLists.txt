install(DIRECTORY vim/indent vim/syntax DESTINATION ${CMAKE_DATA_DIR}/editors/vim)
install(FILES cmake-mode.el DESTINATION ${CMAKE_DATA_DIR}/editors/emacs)
install(FILES cmake.m4 DESTINATION ${CMAKE_XDGDATA_DIR}/aclocal)
add_subdirectory (bash-completion)
