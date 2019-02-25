if(NOT ${RunCMake_SUBTEST_SUFFIX} MATCHES "invalid_.*_var")
  if(GENERATOR_TYPE STREQUAL "RPM")
    function(checkContentPermissions_ FILE REGEX)
      execute_process(COMMAND ${RPM_EXECUTABLE} -qp --dump ${FILE}
                  WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
                  OUTPUT_VARIABLE PERMISSIONS_
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

      if(NOT PERMISSIONS_ MATCHES "${REGEX}")
        message(FATAL_ERROR "Permissions in '${FILE}'. Permissions: '${PERMISSIONS_}'")
      endif()
    endfunction()

    if(${RunCMake_SUBTEST_SUFFIX} MATCHES "CMAKE_var_set")
      checkContentPermissions_("${FOUND_FILE_1}"
        "/usr/foo .*740 root root.*")
    else()
      checkContentPermissions_("${FOUND_FILE_1}"
        "/usr/foo .*700 root root.*")
    endif()
  else() # DEB
    function(checkContentPermissions_ FILE REGEX)
      getPackageContent("${FILE}" PERMISSIONS_)

      if(NOT PERMISSIONS_ MATCHES "${REGEX}")
        message(FATAL_ERROR "Permissions in '${FILE}'. Permissions: '${PERMISSIONS_}'")
      endif()
    endfunction()

    if(${RunCMake_SUBTEST_SUFFIX} MATCHES "CMAKE_var_set")
      checkContentPermissions_("${FOUND_FILE_1}"
        "drwxr----- root/root .* ./usr/\ndrwxr----- root/root .* ./usr/foo/\n.*")
    else()
      checkContentPermissions_("${FOUND_FILE_1}"
        "drwx------ root/root .* ./usr/\ndrwx------ root/root .* ./usr/foo/\n.*")
    endif()
  endif()
endif()
