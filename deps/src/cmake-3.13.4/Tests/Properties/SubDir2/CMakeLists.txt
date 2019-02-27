
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../subdirtest.cxx"
  PROPERTIES COMPILE_DEFINITIONS SUBDIR_TEST)

add_executable(subdirtest "${CMAKE_CURRENT_SOURCE_DIR}/../subdirtest.cxx")
