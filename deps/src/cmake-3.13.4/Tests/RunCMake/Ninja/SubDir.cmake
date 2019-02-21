include(CTest)
add_subdirectory(SubDir)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SubDirSource SubDirBinary)
add_custom_target(TopFail ALL COMMAND does_not_exist)
add_test(NAME TopTest COMMAND ${CMAKE_COMMAND} -E echo "Running TopTest")
install(CODE [[
  message(FATAL_ERROR "Installing Top")
]])
