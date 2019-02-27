if(GENERATOR_TYPE STREQUAL "DEB")
  function(checkDependencies_ FILE REGEX)
    set(whitespaces_ "[\t\n\r ]*")

    getPackageInfo("${FILE}" "FILE_INFO_")
    if(NOT FILE_INFO_ MATCHES "${REGEX}")
      message(FATAL_ERROR "Unexpected dependencies in '${FILE}'; file info: '${FILE_INFO_}'")
    endif()
  endfunction()

  foreach(dependency_type_ DEPENDS CONFLICTS ENHANCES BREAKS REPLACES RECOMMENDS SUGGESTS)
    string(TOLOWER "${dependency_type_}" lower_dependency_type_)
    string(SUBSTRING ${lower_dependency_type_} 1 -1 lower_dependency_type_tail_)
    string(SUBSTRING ${dependency_type_} 0 1 dependency_type_head_)
    set(dependency_type_name_ "${dependency_type_head_}${lower_dependency_type_tail_}")

    checkDependencies_("${FOUND_FILE_1}" ".*${dependency_type_name_}${whitespaces_}:${whitespaces_}${lower_dependency_type_}-application, ${lower_dependency_type_}-application-b.*")
    checkDependencies_("${FOUND_FILE_2}" ".*${dependency_type_name_}${whitespaces_}:${whitespaces_}.*${lower_dependency_type_}-application, ${lower_dependency_type_}-application-b.*")
    checkDependencies_("${FOUND_FILE_3}" ".*${dependency_type_name_}${whitespaces_}:${whitespaces_}${lower_dependency_type_}-headers.*")
    checkDependencies_("${FOUND_FILE_4}" ".*${dependency_type_name_}${whitespaces_}:${whitespaces_}${lower_dependency_type_}-default, ${lower_dependency_type_}-default-b.*")
    checkDependencies_("${FOUND_FILE_5}" ".*${dependency_type_name_}${whitespaces_}:${whitespaces_}${lower_dependency_type_}-default, ${lower_dependency_type_}-default-b.*")
  endforeach()

  checkDependencies_("${FOUND_FILE_1}" ".*Provides${whitespaces_}:${whitespaces_}provided-default, provided-default-b")
  checkDependencies_("${FOUND_FILE_2}" ".*Provides${whitespaces_}:${whitespaces_}provided-default, provided-default-b")
  checkDependencies_("${FOUND_FILE_3}" ".*Provides${whitespaces_}:${whitespaces_}provided-default, provided-default-b")
  checkDependencies_("${FOUND_FILE_4}" ".*Provides${whitespaces_}:${whitespaces_}provided-lib.*")
  checkDependencies_("${FOUND_FILE_5}" ".*Provides${whitespaces_}:${whitespaces_}provided-lib_auto.*, provided-lib_auto-b.*")

  # PREDEPENDS
  checkDependencies_("${FOUND_FILE_1}" ".*Pre-Depends${whitespaces_}:${whitespaces_}predepends-application, predepends-application-b.*")
  checkDependencies_("${FOUND_FILE_2}" ".*Pre-Depends${whitespaces_}:${whitespaces_}.*predepends-application, predepends-application-b.*")
  checkDependencies_("${FOUND_FILE_3}" ".*Pre-Depends${whitespaces_}:${whitespaces_}predepends-headers.*")
  checkDependencies_("${FOUND_FILE_4}" ".*Pre-Depends${whitespaces_}:${whitespaces_}predepends-default, predepends-default-b.*")
  checkDependencies_("${FOUND_FILE_5}" ".*Pre-Depends${whitespaces_}:${whitespaces_}predepends-default, predepends-default-b.*")
elseif(GENERATOR_TYPE STREQUAL "RPM")
  function(checkDependencies_ FILE TYPE COMPARE_LIST)
    set(whitespaces_ "[\t\n\r ]*")

    execute_process(COMMAND ${RPM_EXECUTABLE} -qp --${TYPE} ${FILE}
            WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
            OUTPUT_VARIABLE FILE_DEPENDENCIES_
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REPLACE "\n" ";" FILE_DEPENDENCIES_LIST_ "${FILE_DEPENDENCIES_}")

    foreach(COMPARE_REGEX_ IN LISTS COMPARE_LIST)
      unset(FOUND_)

      foreach(COMPARE_ IN LISTS FILE_DEPENDENCIES_LIST_)
        if(COMPARE_ MATCHES "${COMPARE_REGEX_}")
          set(FOUND_ true)
          break()
        endif()
      endforeach()

      if(NOT FOUND_)
        message(FATAL_ERROR "Missing dependencies in '${FILE}'; check type: '${TYPE}'; file info: '${FILE_DEPENDENCIES_}'; missing: '${COMPARE_REGEX_}'")
      endif()
    endforeach()
  endfunction()

  # TODO add tests for what should not be present in lists
  checkDependencies_("${FOUND_FILE_1}" "requires" "depend-application;depend-application-b")
  checkDependencies_("${FOUND_FILE_2}" "requires" "depend-application;depend-application-b;libtest_lib\\.so.*")
  checkDependencies_("${FOUND_FILE_3}" "requires" "depend-headers")
  checkDependencies_("${FOUND_FILE_4}" "requires" "depend-default;depend-default-b")
  checkDependencies_("${FOUND_FILE_5}" "requires" "depend-default;depend-default-b")

  checkDependencies_("${FOUND_FILE_1}" "conflicts" "conflicts-application;conflicts-application-b")
  checkDependencies_("${FOUND_FILE_2}" "conflicts" "conflicts-application;conflicts-application-b")
  checkDependencies_("${FOUND_FILE_3}" "conflicts" "conflicts-headers")
  checkDependencies_("${FOUND_FILE_4}" "conflicts" "conflicts-default;conflicts-default-b")
  checkDependencies_("${FOUND_FILE_5}" "conflicts" "conflicts-default;conflicts-default-b")

  checkDependencies_("${FOUND_FILE_1}" "provides" "provided-default;provided-default-b")
  checkDependencies_("${FOUND_FILE_2}" "provides" "provided-default;provided-default-b")
  checkDependencies_("${FOUND_FILE_3}" "provides" "provided-default;provided-default-b")
  checkDependencies_("${FOUND_FILE_4}" "provides" "provided-lib")
  checkDependencies_("${FOUND_FILE_5}" "provides" "provided-lib_auto;provided-lib_auto-b")
endif()
