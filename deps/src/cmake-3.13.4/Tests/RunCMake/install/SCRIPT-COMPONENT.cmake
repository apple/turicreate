install(
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/install_script.cmake"
  CODE "write_empty_file(empty2.txt)"
  COMPONENT dev
  )
