function (check_source_file_property file prop)
  get_source_file_property(gsfp_val "${file}" "${prop}")
  get_property(gp_val
    SOURCE "${file}"
    PROPERTY "${prop}")

  message("get_source_file_property: -->${gsfp_val}<--")
  message("get_property: -->${gp_val}<--")
endfunction ()

set_source_files_properties(file.c PROPERTIES empty "" custom value)

check_source_file_property(file.c empty)
check_source_file_property(file.c custom)
check_source_file_property(file.c noexist)
