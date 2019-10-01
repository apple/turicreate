install(FILES CMakeLists.txt DESTINATION foo COMPONENT foo)
install(FILES CMakeLists.txt DESTINATION bar COMPONENT bar)
install(FILES CMakeLists.txt DESTINATION bas COMPONENT bas)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/preinst "echo default_preinst")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/prerm "echo default_prerm")

foreach(file_ preinst prerm)
  file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/${file_}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE
      WORLD_READ WORLD_EXECUTE)
endforeach()

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_BINARY_DIR}/preinst;${CMAKE_CURRENT_BINARY_DIR}/prerm;${CMAKE_CURRENT_BINARY_DIR}/conffiles")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bar_tmp/preinst "echo bar_preinst")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bar_tmp/prerm "echo bar_prerm")

foreach(file_ preinst prerm)
  # not acceptable permissions for lintian but we need to check that
  # permissions are preserved
  file(COPY ${CMAKE_CURRENT_BINARY_DIR}/bar_tmp/${file_}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bar
    FILE_PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE)
endforeach()

set(CPACK_DEBIAN_BAR_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_BINARY_DIR}/bar/preinst;${CMAKE_CURRENT_BINARY_DIR}/bar/prerm")

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
