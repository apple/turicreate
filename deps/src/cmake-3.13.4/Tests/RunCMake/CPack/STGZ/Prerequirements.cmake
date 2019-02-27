function(get_test_prerequirements found_var config_file)
  if(EXISTS "/bin/sh")
    #gunzip is not part of posix so we should not rely on it being installed
    find_program(GUNZIP_EXECUTABLE gunzip)

    if(GUNZIP_EXECUTABLE)
      file(WRITE "${config_file}" "")
      set(${found_var} true PARENT_SCOPE)
    endif()
  endif()
endfunction()
