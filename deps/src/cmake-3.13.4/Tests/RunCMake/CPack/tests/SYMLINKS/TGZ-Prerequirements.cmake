function(get_test_prerequirements found_var config_file)
  if(UNIX) # limit test to platforms that support symlinks
    set(${found_var} true PARENT_SCOPE)
  endif()
endfunction()
