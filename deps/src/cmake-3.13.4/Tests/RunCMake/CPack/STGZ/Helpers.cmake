set(ALL_FILES_GLOB "*.sh")

function(getPackageContent FILE RESULT_VAR)
  get_filename_component(path_ "${FILE}" DIRECTORY)
  file(REMOVE_RECURSE "${path_}/content")
  file(MAKE_DIRECTORY "${path_}/content")
  execute_process(COMMAND ${FILE} --prefix=${path_}/content --include-subdir
          RESULT_VARIABLE extract_result_
          ERROR_VARIABLE extract_error_
          OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(extract_result_)
    message(FATAL_ERROR "Extracting STGZ archive failed: '${extract_result_}';"
          " '${extract_error_}'.")
  endif()

  file(GLOB_RECURSE package_content_ LIST_DIRECTORIES true RELATIVE
      "${path_}/content" "${path_}/content/*")

  set(${RESULT_VAR} "${package_content_}" PARENT_SCOPE)
endfunction()

function(getPackageNameGlobexpr NAME COMPONENT VERSION REVISION FILE_NO RESULT_VAR)
  if(COMPONENT)
    set(COMPONENT "-${COMPONENT}")
  endif()

  set(${RESULT_VAR} "${NAME}-${VERSION}-*${COMPONENT}.sh" PARENT_SCOPE)
endfunction()

function(getPackageContentList FILE RESULT_VAR)
  getPackageContent("${FILE}" package_content_)

  set(${RESULT_VAR} "${package_content_}" PARENT_SCOPE)
endfunction()

function(toExpectedContentList FILE_NO CONTENT_VAR)
  findExpectedFile("${FILE_NO}" "file_" "glob_expr_")

  get_filename_component(prefix_ "${file_}" NAME)
  # NAME_WE removes everything after the dot and dot is in version so replace instead
  string(REPLACE ".sh" "" prefix_ "${prefix_}")

  if(NOT PACKAGING_TYPE STREQUAL "MONOLITHIC")
    # STGZ packages don't have component dir prefix in subdir
    string(FIND "${prefix_}" "-" pos_ REVERSE)
    string(SUBSTRING "${prefix_}" 0 ${pos_} prefix_)
  endif()

    # add install prefix to expected paths
  if(DEFINED EXPECTED_FILE_${FILE_NO}_PACKAGING_PREFIX)
    set(EXPECTED_FILE_PACKAGING_PREFIX
      "${EXPECTED_FILE_${FILE_NO}_PACKAGING_PREFIX}")
  elseif(NOT DEFINED EXPECTED_FILE_PACKAGING_PREFIX)
    # default CPack Archive packaging install prefix
    set(EXPECTED_FILE_PACKAGING_PREFIX "/")
  endif()

  # remove trailing slash otherwise path concatenation will cause double slashes
  string(REGEX REPLACE "/$" "" EXPECTED_FILE_PACKAGING_PREFIX
    "${EXPECTED_FILE_PACKAGING_PREFIX}")

  if(EXPECTED_FILE_PACKAGING_PREFIX)
    set(prepared_ "${prefix_}")
  else()
    unset(prepared_)
  endif()

  list(APPEND prepared_ "${prefix_}${EXPECTED_FILE_PACKAGING_PREFIX}")
  foreach(part_ IN LISTS ${CONTENT_VAR})
    list(APPEND prepared_ "${prefix_}${EXPECTED_FILE_PACKAGING_PREFIX}${part_}")
  endforeach()

  set(${CONTENT_VAR} "${prepared_}" PARENT_SCOPE)
endfunction()
