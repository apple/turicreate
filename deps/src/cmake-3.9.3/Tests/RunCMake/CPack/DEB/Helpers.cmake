set(ALL_FILES_GLOB "*.deb")

function(getPackageContent FILE RESULT_VAR)
  execute_process(COMMAND ${DPKG_EXECUTABLE} -c "${FILE}"
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
    set(${RESULT_VAR} "${NAME}${COMPONENT}_${VERSION}-${REVISION}_*.deb" PARENT_SCOPE)
  else()
    set(${RESULT_VAR} "${NAME}-${VERSION}-*${COMPONENT}.deb" PARENT_SCOPE)
  endif()
endfunction()

function(getPackageContentList FILE RESULT_VAR)
  execute_process(COMMAND ${DPKG_EXECUTABLE} -c "${FILE}"
          OUTPUT_VARIABLE package_content_
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

  unset(items_)
  string(REPLACE "\n" ";" package_content_ "${package_content_}")
  foreach(i_ IN LISTS package_content_)
    string(REGEX REPLACE "^.* \.(/[^$]*)$" "\\1" result_ "${i_}")
    string(REGEX REPLACE "/$" "" result_ "${result_}")
    list(APPEND items_ "${result_}")
  endforeach()

  set(${RESULT_VAR} "${items_}" PARENT_SCOPE)
endfunction()

function(toExpectedContentList FILE_NO CONTENT_VAR)
  # no need to do anything
endfunction()

function(getMissingShlibsErrorExtra FILE RESULT_VAR)
    execute_process(COMMAND ${DPKG_EXECUTABLE} -x "${FILE}" data_${PREFIX}
            ERROR_VARIABLE err_)

    if(err_)
      set(error_extra " Extra: Could not unpack package content: '${err}'")
    else()
      cmake_policy(PUSH)
        # Tell file(GLOB_RECURSE) not to follow directory symlinks
        # even if the project does not set this policy to NEW.
        cmake_policy(SET CMP0009 NEW)
        file(GLOB_RECURSE FILE_PATHS_ LIST_DIRECTORIES false "${CMAKE_CURRENT_BINARY_DIR}/data_${PREFIX}/*")
      cmake_policy(POP)

      # get file info so that we can determine if file is executable or not
      foreach(FILE_ IN LISTS FILE_PATHS_)
        execute_process(COMMAND file "${FILE_}"
          WORKING_DIRECTORY "${WDIR}"
          OUTPUT_VARIABLE INSTALL_FILE_
          ERROR_VARIABLE err_)

        if(NOT err_)
          list(APPEND deb_install_files "${INSTALL_FILE_}")
        else()
          list(APPEND deb_install_files_errors "'${FILE_}': '${err_}'\n")
        endif()
      endforeach()

      set(error_extra " Extra: install files '${deb_install_files}'")

      if(deb_install_files_errors)
        string(APPEND error_extra "; errors \"${deb_install_files_errors}\"")
      endif()

      if(READELF_EXECUTABLE)
        string(APPEND error_extra "; readelf \"\n")

        # Only dynamically linked ELF files are included
        # Extract only file name infront of ":"
        foreach(_FILE IN LISTS deb_install_files)
          if(_FILE MATCHES "ELF.*shared object")
            string(REGEX MATCH "(^.*):" _FILE_NAME "${_FILE}")

            execute_process(COMMAND ${READELF_EXECUTABLE} -d "${CMAKE_MATCH_1}"
              WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
              RESULT_VARIABLE result
              OUTPUT_VARIABLE output
              ERROR_VARIABLE err_
              OUTPUT_STRIP_TRAILING_WHITESPACE)

            string(APPEND error_extra " name '${CMAKE_MATCH_1}'\n result '${result}'\n output '${output}'\n error '${err_}'\n")
          endif()
        endforeach()

        string(APPEND error_extra "\"")
      else()
        string(APPEND error_extra "; error readelf missing")
      endif()
    endif()

    set(${RESULT_VAR} "${error_extra}" PARENT_SCOPE)
endfunction()

function(verifyDebControl FILE PREFIX VERIFY_FILES)
  execute_process(COMMAND ${DPKG_EXECUTABLE} --control "${FILE}" control_${PREFIX}
          ERROR_VARIABLE err_)

  if(err_)
    message(FATAL_ERROR "Debian control verification failed for file: "
        "'${FILE}'; error output: '${err_}'")
  endif()

  foreach(FILE_ IN LISTS VERIFY_FILES)
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/control_${PREFIX}/${FILE_}")
      if(FILE_ STREQUAL "shlibs")
        getMissingShlibsErrorExtra("${FILE}" error_extra)
      endif()

      message(FATAL_ERROR "Expected Debian control file does not exist: '${FILE_}'${error_extra}")
    endif()

    file(READ "${CMAKE_CURRENT_BINARY_DIR}/control_${PREFIX}/${FILE_}" content_)
    if(NOT content_ MATCHES "${${PREFIX}_${FILE_}}")
      message(FATAL_ERROR "Unexpected content in for '${PREFIX}_${FILE_}'!"
          " Content: '${content_}'")
    endif()

    execute_process(COMMAND ls -l "${CMAKE_CURRENT_BINARY_DIR}/control_${PREFIX}/${FILE_}"
          OUTPUT_VARIABLE package_permissions_
          ERROR_VARIABLE package_permissions_error_
          OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT package_permissions_error_)
      if(NOT package_permissions_ MATCHES "${${PREFIX}_${FILE_}_permissions_regex}")
        message(FATAL_ERROR "Unexpected file permissions for ${PREFIX}_${FILE_}: '${package_permissions_}'!")
      endif()
    else()
      message(FATAL_ERROR "Listing file permissions failed (${package_permissions_error_})!")
    endif()
  endforeach()
endfunction()

function(getPackageInfo FILE RESULT_VAR)
  execute_process(COMMAND ${DPKG_EXECUTABLE} -I ${FILE}
          WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
          OUTPUT_VARIABLE package_info_
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(${RESULT_VAR} "${package_info_}" PARENT_SCOPE)
endfunction()
