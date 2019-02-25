function(get_test_prerequirements found_var config_file)
  file(WRITE "${config_file}" "")
  set(${found_var} true PARENT_SCOPE)
endfunction()
