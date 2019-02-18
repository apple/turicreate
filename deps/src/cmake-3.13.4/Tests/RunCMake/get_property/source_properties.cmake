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

# Test strange legacy behavior in which the order in which source files are
# first accessed affects how properties are applied without an extension.
# See also issue #15208.
get_property(lang SOURCE ${CMAKE_CURRENT_BINARY_DIR}/file2.c PROPERTY LANGUAGE)
get_property(lang SOURCE ${CMAKE_CURRENT_BINARY_DIR}/file2.h PROPERTY LANGUAGE)
set_property(SOURCE file2 PROPERTY custom value) # set property without extension
check_source_file_property(file2 custom)   # should have property
check_source_file_property(file2.h custom) # should not have property
check_source_file_property(file2.c custom) # should have property
