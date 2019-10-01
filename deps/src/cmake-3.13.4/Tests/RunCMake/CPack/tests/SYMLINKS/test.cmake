install(DIRECTORY DESTINATION empty_dir COMPONENT links)
install(FILES CMakeLists.txt DESTINATION non_empty_dir COMPONENT links)

# test symbolic link to an empty dir
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink empty_dir symlink_to_empty_dir)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_to_empty_dir DESTINATION "." COMPONENT links)

# test symbolic link to a non empty dir
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink non_empty_dir symlink_to_non_empty_dir)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_to_non_empty_dir DESTINATION "." COMPONENT links)

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(CPACK_COMPONENTS_ALL links)
endif()
