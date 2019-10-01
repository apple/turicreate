add_custom_target(SubDirSourceFail COMMAND does_not_exist)
add_custom_target(SubDirSourceInAll ALL COMMAND ${CMAKE_COMMAND} -E echo "Building SubDirSourceInAll")
add_test(NAME SubDirSourceTest COMMAND ${CMAKE_COMMAND} -E echo "Running SubDirSourceTest")
install(CODE [[
  message(STATUS "Installing SubDirSource")
]])
