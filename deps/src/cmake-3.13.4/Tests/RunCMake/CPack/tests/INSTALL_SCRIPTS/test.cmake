if(GENERATOR_TYPE STREQUAL "RPM")
  set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/pre_install.sh")
  set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/post_install.sh")
  set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/pre_uninstall.sh")
  set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/post_uninstall.sh")

  set(CPACK_RPM_foo_PRE_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/pre_install_foo.sh")
  set(CPACK_RPM_foo_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/post_install_foo.sh")
  set(CPACK_RPM_foo_PRE_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/pre_uninstall_foo.sh")
  set(CPACK_RPM_foo_POST_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/post_uninstall_foo.sh")
endif()

set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# default
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pre_install.sh"
    "echo \"pre install\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/post_install.sh"
    "echo \"post install\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pre_uninstall.sh"
    "echo \"pre uninstall\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/post_uninstall.sh"
    "echo \"post uninstall\"\n")

# specific
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pre_install_foo.sh"
    "echo \"pre install foo\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/post_install_foo.sh"
    "echo \"post install foo\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pre_uninstall_foo.sh"
    "echo \"pre uninstall foo\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/post_uninstall_foo.sh"
    "echo \"post uninstall foo\"\n")

install(FILES CMakeLists.txt DESTINATION foo COMPONENT foo)
install(FILES CMakeLists.txt DESTINATION bar COMPONENT bar)
