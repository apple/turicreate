set(ALL_FILES_GLOB "*.rpm")

function(getPackageContent FILE RESULT_VAR)
  execute_process(COMMAND ${RPM_EXECUTABLE} -pql ${FILE}
          OUTPUT_VARIABLE package_content_
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(${RESULT_VAR} "${package_content_}" PARENT_SCOPE)
endfunction()

function(getPackageNameGlobexpr NAME COMPONENT VERSION REVISION FILE_NO RESULT_VAR)
  if(COMPONENT)
    set(COMPONENT "-${COMPONENT}")
  endif()

  if(DEFINED EXPECTED_FILE_${FILE_NO}_FILENAME_GENERATOR_SPECIFIC_FORMAT)
    set(GENERATOR_SPECIFIC_FORMAT "${EXPECTED_FILE_${FILE_NO}_FILENAME_GENERATOR_SPECIFIC_FORMAT}")
  elseif(DEFINED EXPECTED_FILES_NAME_GENERATOR_SPECIFIC_FORMAT)
    set(GENERATOR_SPECIFIC_FORMAT "${EXPECTED_FILES_NAME_GENERATOR_SPECIFIC_FORMAT}")
  else()
    set(GENERATOR_SPECIFIC_FORMAT FALSE)
  endif()

  if(GENERATOR_SPECIFIC_FORMAT)
    if(NOT REVISION)
      set(REVISION "1")
    endif()
    set(${RESULT_VAR} "${NAME}${COMPONENT}-${VERSION}-${REVISION}.*.rpm" PARENT_SCOPE)
  else()
    set(${RESULT_VAR} "${NAME}-${VERSION}-*${COMPONENT}.rpm" PARENT_SCOPE)
  endif()
endfunction()

function(getPackageContentList FILE RESULT_VAR)
  execute_process(COMMAND ${RPM_EXECUTABLE} -pql ${FILE}
          OUTPUT_VARIABLE package_content_
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "\n" ";" package_content_ "${package_content_}")

  # never versions of rpmbuild (introduced in rpm 4.13.0.1) add build_id links
  # to packages - tests should ignore them
  list(FILTER package_content_ EXCLUDE REGEX ".*\.build-id.*")

  set(${RESULT_VAR} "${package_content_}" PARENT_SCOPE)
endfunction()

function(toExpectedContentList FILE_NO CONTENT_VAR)
  # add install prefix to expected paths
  if(DEFINED EXPECTED_FILE_${FILE_NO}_PACKAGING_PREFIX)
    set(EXPECTED_FILE_PACKAGING_PREFIX
      "${EXPECTED_FILE_${FILE_NO}_PACKAGING_PREFIX}")
  elseif(NOT DEFINED EXPECTED_FILE_PACKAGING_PREFIX)
    # default CPackRPM packaging install prefix
    set(EXPECTED_FILE_PACKAGING_PREFIX "/usr")
  endif()
  set(prepared_ "${EXPECTED_FILE_PACKAGING_PREFIX}")
  foreach(part_ IN LISTS ${CONTENT_VAR})
    list(APPEND prepared_ "${EXPECTED_FILE_PACKAGING_PREFIX}${part_}")
  endforeach()

  # remove paths that are excluded from auto packaging
  if(NOT DEFINED CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST)
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST
      /etc /etc/init.d /usr /usr/bin /usr/include /usr/lib
      /usr/libx32 /usr/lib64 /usr/share /usr/share/aclocal /usr/share/doc)
  endif()
  unset(filtered_)
  foreach(part_ IN LISTS prepared_)
    unset(dont_add_)
    foreach(for_removal_ IN LISTS CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST)
      if(part_ STREQUAL for_removal_)
        set(dont_add_ TRUE)
        break()
      endif()
    endforeach()

    if(NOT dont_add_)
      list(APPEND filtered_ "${part_}")
    endif()
  endforeach()

  set(${CONTENT_VAR} "${filtered_}" PARENT_SCOPE)
endfunction()

function(getPackageInfo FILE RESULT_VAR)
  execute_process(COMMAND ${RPM_EXECUTABLE} -pqi ${FILE}
          OUTPUT_VARIABLE info_content
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(${RESULT_VAR} "${info_content}" PARENT_SCOPE)
endfunction()
