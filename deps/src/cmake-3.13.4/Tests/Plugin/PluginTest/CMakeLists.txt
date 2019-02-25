cmake_minimum_required(VERSION 2.8)

project(PluginTest)

include(${CMAKE_CURRENT_BINARY_DIR}/../mods.cmake)
get_property(configs TARGET exp_example_mod_1 PROPERTY IMPORTED_CONFIGURATIONS)
foreach(c ${configs})
  string(TOUPPER "${c}" CONFIG)
  get_property(soname TARGET exp_example_mod_1 PROPERTY IMPORTED_NO_SONAME_${CONFIG})
  if(soname)
    message(STATUS "exp_example_mod_1 has IMPORTED_NO_SONAME_${CONFIG} as expected: ${soname}")
  else()
    message(SEND_ERROR "exp_example_mod_1 does not have IMPORTED_NO_SONAME_${CONFIG} but should")
  endif()
endforeach()

# Parse the binary to check for SONAME if possible.
if("${CMAKE_EXECUTABLE_FORMAT}" MATCHES "ELF")
  find_program(READELF_EXE readelf)
  if(READELF_EXE)
    add_custom_target(check_mod_soname ALL COMMAND
      ${CMAKE_COMMAND} -Dreadelf=${READELF_EXE}
                       -Dmod=$<TARGET_FILE:exp_example_mod_1>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/../check_mod_soname.cmake
      )
  endif()
endif()
