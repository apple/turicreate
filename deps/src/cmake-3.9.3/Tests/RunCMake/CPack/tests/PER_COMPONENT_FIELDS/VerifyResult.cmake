function(checkPackageInfo_ TYPE FILE REGEX)
  getPackageInfo("${FILE}" "FILE_INFO_")
  if(NOT FILE_INFO_ MATCHES "${REGEX}")
    message(FATAL_ERROR "Unexpected ${TYPE} in '${FILE}'; file info: '${FILE_INFO_}'")
  endif()
endfunction()

if(GENERATOR_TYPE STREQUAL "DEB")
  set(name_ "Package")
  set(group_ "Section")
elseif(GENERATOR_TYPE STREQUAL "RPM")
  set(name_ "Name")
  set(group_ "Group")
endif()

set(whitespaces_ "[\t\n\r ]*")

# check package name
checkPackageInfo_("name" "${FOUND_FILE_1}" ".*${name_}${whitespaces_}:${whitespaces_}per_component_fields-pkg_1")
checkPackageInfo_("name" "${FOUND_FILE_2}" ".*${name_}${whitespaces_}:${whitespaces_}second")
checkPackageInfo_("name" "${FOUND_FILE_3}" ".*${name_}${whitespaces_}:${whitespaces_}per_component_fields-pkg_3")

# check package group
checkPackageInfo_("group" "${FOUND_FILE_1}" ".*${group_}${whitespaces_}:${whitespaces_}default")
checkPackageInfo_("group" "${FOUND_FILE_2}" ".*${group_}${whitespaces_}:${whitespaces_}second_group")
checkPackageInfo_("group" "${FOUND_FILE_3}" ".*${group_}${whitespaces_}:${whitespaces_}default")
