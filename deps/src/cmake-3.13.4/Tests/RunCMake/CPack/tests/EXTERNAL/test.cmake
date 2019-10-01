include(CPackComponent)

if(RunCMake_SUBTEST_SUFFIX STREQUAL "none")
  unset(CPACK_EXTERNAL_REQUESTED_VERSIONS)
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "good")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "1.0")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "good_multi")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "1.0;2.0")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "bad_major")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "2.0")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "bad_minor")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "1.1")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "invalid_good")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "1;1.0")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "invalid_bad")
  set(CPACK_EXTERNAL_REQUESTED_VERSIONS "1")
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "stage_and_package")
  set(CPACK_EXTERNAL_ENABLE_STAGING 1)
  set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/create_package.cmake")
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/f1.txt" test1)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/f2.txt" test2)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/f3.txt" test3)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/f4.txt" test4)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/f1.txt" DESTINATION share/cpack-test COMPONENT f1)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/f2.txt" DESTINATION share/cpack-test COMPONENT f2)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/f3.txt" DESTINATION share/cpack-test COMPONENT f3)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/f4.txt" DESTINATION share/cpack-test COMPONENT f4)

cpack_add_component(f1
  DISPLAY_NAME "File 1"
  DESCRIPTION "Component for file 1"
  GROUP f12
  INSTALL_TYPES full f12
)

cpack_add_component(f2
  DISPLAY_NAME "File 2"
  DESCRIPTION "Component for file 2"
  GROUP f12
  DEPENDS f1
  INSTALL_TYPES full f12
)

cpack_add_component(f3
  DISPLAY_NAME "File 3"
  DESCRIPTION "Component for file 3"
  GROUP f34
  DEPENDS f1 f2
  INSTALL_TYPES full
)

cpack_add_component(f4
  DISPLAY_NAME "File 4"
  DESCRIPTION "Component for file 4"
  GROUP f34
  DEPENDS f2 f3 f1
  INSTALL_TYPES full
)

cpack_add_component_group(f12
  DISPLAY_NAME "Files 1 and 2"
  DESCRIPTION "Component group for files 1 and 2"
  PARENT_GROUP f1234
)

cpack_add_component_group(f34
  DISPLAY_NAME "Files 3 and 4"
  DESCRIPTION "Component group for files 3 and 4"
  PARENT_GROUP f1234
)

cpack_add_component_group(f1234
  DISPLAY_NAME "Files 1-4"
  DESCRIPTION "Component group for all files"
)

cpack_add_install_type(full
  DISPLAY_NAME "Full installation"
)

cpack_add_install_type(f12
  DISPLAY_NAME "Only files 1 and 2"
)
