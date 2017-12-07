set(whitespaces "[\\t\\n\\r ]*")

#######################
# verify generated symbolic links
#######################
file(GLOB_RECURSE symlink_files RELATIVE "${bin_dir}" "${bin_dir}/*/symlink_*")

foreach(check_symlink IN LISTS symlink_files)
  get_filename_component(symlink_name "${check_symlink}" NAME)
  execute_process(COMMAND ls -la "${check_symlink}"
            WORKING_DIRECTORY "${bin_dir}"
            OUTPUT_VARIABLE SYMLINK_POINT_
            OUTPUT_STRIP_TRAILING_WHITESPACE)

  if("${symlink_name}" STREQUAL "symlink_to_empty_dir")
    string(REGEX MATCH "^.*${whitespaces}->${whitespaces}empty_dir$" check_symlink "${SYMLINK_POINT_}")
  elseif("${symlink_name}" STREQUAL "symlink_to_non_empty_dir")
    string(REGEX MATCH "^.*${whitespaces}->${whitespaces}non_empty_dir$" check_symlink "${SYMLINK_POINT_}")
  else()
    message(FATAL_ERROR "error: unexpected rpm symbolic link '${check_symlink}'")
  endif()

  if(NOT check_symlink)
    message(FATAL_ERROR "symlink points to unexpected location '${SYMLINK_POINT_}'")
  endif()
endforeach()
