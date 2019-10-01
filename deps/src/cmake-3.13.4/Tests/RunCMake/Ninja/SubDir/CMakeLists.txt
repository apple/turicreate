add_custom_target(SubFail COMMAND does_not_exist)
add_custom_target(InAll ALL COMMAND ${CMAKE_COMMAND} -E echo "Building InAll")
add_test(NAME SubDirTest COMMAND ${CMAKE_COMMAND} -E echo "Running SubDirTest")
install(CODE [[
  message(STATUS "Installing SubDir")
]])
