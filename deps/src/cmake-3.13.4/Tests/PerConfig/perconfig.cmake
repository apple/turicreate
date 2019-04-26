# Print values for human reference.
foreach(v
    configuration
    perconfig_file_dir
    perconfig_file_name
    perconfig_file
    pcStatic_file
    pcStatic_linker_file
    pcShared_file
    pcShared_linker_file
    pcShared_soname_file
    )
  message(STATUS "${v}=${${v}}")
endforeach()

# Verify that file names match as expected.
set(pc_file_components ${perconfig_file_dir}/${perconfig_file_name})
if(NOT "${pc_file_components}" STREQUAL "${perconfig_file}")
  message(SEND_ERROR
    "File components ${pc_file_components} do not match ${perconfig_file}")
endif()
if(NOT "${pcStatic_file}" STREQUAL "${pcStatic_linker_file}")
  message(SEND_ERROR
    "pcStatic_file does not match pcStatic_linker_file:\n"
    "  ${pcStatic_file}\n"
    "  ${pcStatic_linker_file}\n"
    )
endif()

# Verify that the implementation files are named correctly.
foreach(lib pcStatic pcShared)
  file(STRINGS "${${lib}_file}" info LIMIT_COUNT 1 REGEX "INFO:[A-Za-z0-9_]+\\[[^]]*\\]")
  if(NOT "${info}" MATCHES "INFO:symbol\\[${lib}\\]")
    message(SEND_ERROR "No INFO:symbol[${lib}] found in:\n  ${${lib}_file}")
  endif()
endforeach()
execute_process(COMMAND ${perconfig_file} RESULT_VARIABLE result)
if(result)
  message(SEND_ERROR "Error running:\n  ${perconfig_file}\n(${result})")
endif()
