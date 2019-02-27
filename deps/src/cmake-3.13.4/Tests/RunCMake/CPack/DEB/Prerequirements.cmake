function(get_test_prerequirements found_var config_file)
  find_program(DPKG_EXECUTABLE dpkg)

  if(DPKG_EXECUTABLE)
    file(WRITE "${config_file}" "set(DPKG_EXECUTABLE \"${DPKG_EXECUTABLE}\")")
    set(${found_var} true PARENT_SCOPE)
  endif()

  # optional tool for some tests
  find_program(FAKEROOT_EXECUTABLE fakeroot)
  if(FAKEROOT_EXECUTABLE)
    file(APPEND "${config_file}"
      "\nset(FAKEROOT_EXECUTABLE \"${FAKEROOT_EXECUTABLE}\")")
  endif()

  # optional tool for some tests
  find_program(READELF_EXECUTABLE NAMES readelf)
  if(READELF_EXECUTABLE)
    file(APPEND "${config_file}"
      "\nset(READELF_EXECUTABLE \"${READELF_EXECUTABLE}\")")
  endif()
endfunction()
