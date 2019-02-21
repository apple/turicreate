function (check_install_property file prop)
  get_property(gp_val
    INSTALL "${file}"
    PROPERTY "${prop}")

  message("get_property: -->${gp_val}<--")
endfunction ()

install(
  FILES "${CMAKE_CURRENT_LIST_FILE}"
  DESTINATION "${CMAKE_CURRENT_LIST_DIR}"
  RENAME "installed-file-dest")
set_property(INSTALL "${CMAKE_CURRENT_LIST_FILE}" PROPERTY empty "")
set_property(INSTALL "${CMAKE_CURRENT_LIST_FILE}" PROPERTY custom value)

check_install_property("${CMAKE_CURRENT_LIST_FILE}" empty)
check_install_property("${CMAKE_CURRENT_LIST_FILE}" custom)
check_install_property("${CMAKE_CURRENT_LIST_FILE}" noexist)
