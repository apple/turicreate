# Check that file contains line that matches regular expression.
# Sets IS_FOUND variable to TRUE if found and to FALSE otherwise.
macro(check_line_exists TARGET_FILE REG_EXP_REF)
  set(IS_FOUND "FALSE")

  file(STRINGS ${TARGET_FILE} FOUND_LINE LIMIT_COUNT 1 REGEX "${${REG_EXP_REF}}")
  list(LENGTH FOUND_LINE _VAR_LEN)

  if(_VAR_LEN GREATER 0)
    set(IS_FOUND "TRUE")
  endif()
endmacro()

# Search and parse project section line by project name.
# If search was successful stores found type and guid into FOUND_TYPE and FOUND_GUID variables respectively.
# Sets IS_FOUND variable to TRUE if found and to FALSE otherwise.
macro(parse_project_section TARGET_FILE PROJECT_NAME)
  set(REG_EXP "^Project\\(\\\"{(.+)}\\\"\\) = \\\"${PROJECT_NAME}\\\", \\\".+\\..+\\\", \\\"{(.+)}\\\"$")

  check_line_exists(${TARGET_FILE} REG_EXP)
  if(NOT IS_FOUND)
    return()
  endif()

  string(REGEX REPLACE "${REG_EXP}" "\\1;\\2" _GUIDS "${FOUND_LINE}")

  list(GET _GUIDS 0 FOUND_TYPE)
  list(GET _GUIDS 1 FOUND_GUID)
endmacro()

# Search project section line by project name and type.
# Returns TRUE if found and FALSE otherwise
function(check_project_type TARGET_FILE PROJECT_NAME PROJECT_TYPE RESULT)
  set(${RESULT} "FALSE" PARENT_SCOPE)

  parse_project_section(${TARGET_FILE} ${PROJECT_NAME})
  if(IS_FOUND AND FOUND_TYPE STREQUAL PROJECT_TYPE)
    set(${RESULT} "TRUE" PARENT_SCOPE)
  endif()
endfunction()


# Search project section line by project name and id.
# Returns TRUE if found and FALSE otherwise
function(check_project_guid TARGET_FILE PROJECT_NAME PROJECT_GUID RESULT)
  set(${RESULT} "FALSE" PARENT_SCOPE)

  parse_project_section(${TARGET_FILE} ${PROJECT_NAME})
  if(IS_FOUND AND FOUND_GUID STREQUAL PROJECT_GUID)
    set(${RESULT} "TRUE" PARENT_SCOPE)
  endif()
endfunction()


# Search project's build configuration line by project name and target platform name.
# Returns TRUE if found and FALSE otherwise
function(check_custom_platform TARGET_FILE PROJECT_NAME PLATFORM_NAME RESULT)
  set(${RESULT} "FALSE" PARENT_SCOPE)

  # extract project guid
  parse_project_section(${TARGET_FILE} ${PROJECT_NAME})
  if(NOT IS_FOUND)
    return()
  endif()

  # probably whould be better to use configuration name
  # extracted from CMAKE_CONFIGURATION_TYPES than just hardcoded "Debug" instead
  set(REG_EXP "^(\t)*\\{${FOUND_GUID}\\}\\.Debug[^ ]*\\.ActiveCfg = Debug\\|${PLATFORM_NAME}$")
  check_line_exists(${TARGET_FILE} REG_EXP)

  set(${RESULT} ${IS_FOUND} PARENT_SCOPE)
endfunction()

# Search project's build configuration line by project name and target configuration name.
# Returns TRUE if found and FALSE otherwise
function(check_custom_configuration TARGET_FILE PROJECT_NAME SLN_CONFIG DST_CONFIG RESULT)
  set(${RESULT} "FALSE" PARENT_SCOPE)
  # extract project guid
  parse_project_section(${TARGET_FILE} ${PROJECT_NAME})
  if(NOT IS_FOUND)
    return()
  endif()

  set(REG_EXP "^(\t)*\\{${FOUND_GUID}\\}\\.${SLN_CONFIG}[^ ]*\\.ActiveCfg = ${DST_CONFIG}\\|.*$")
  check_line_exists(${TARGET_FILE} REG_EXP)

  set(${RESULT} ${IS_FOUND} PARENT_SCOPE)
endfunction()

# RunCMake test check helper
function(check_project test name guid type platform imported_release_config_name)
  set(sln "${RunCMake_TEST_BINARY_DIR}/${test}.sln")
  set(sep "")
  set(failed "")
  if(NOT type)
    set(type 8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942)
  endif()
  if(NOT platform)
    if(RunCMake_GENERATOR_PLATFORM)
      set(platform "${RunCMake_GENERATOR_PLATFORM}")
    elseif("${RunCMake_GENERATOR}" MATCHES "Win64")
      set(platform "x64")
    else()
      set(platform "Win32")
    endif()
  endif()
  if(NOT imported_release_config_name)
    set(imported_release_config_name "Release")
  endif()
  if(guid)
    check_project_guid("${sln}" "${name}" "${guid}" passed_guid)
    if(NOT passed_guid)
      string(APPEND failed "${sep}${name} solution has no project with expected GUID=${guid}")
      set(sep "\n")
    endif()
  else()
    set(passed_guid 1)
  endif()
  check_project_type("${sln}" "${name}" "${type}" passed_type)
  if(NOT passed_type)
    string(APPEND failed "${sep}${name} solution has no project with expected TYPE=${type}")
    set(sep "\n")
  endif()
  check_custom_platform("${sln}" "${name}" "${platform}" passed_platform)
  if(NOT passed_platform)
    string(APPEND failed "${sep}${name} solution has no project with expected PLATFORM=${platform}")
    set(sep "\n")
  endif()
  check_custom_configuration("${sln}" "${name}" "Release" "${imported_release_config_name}" passed_configuration)
  if(NOT passed_configuration)
    string(APPEND failed "${sep}${name} solution has no project with expected CONFIG=${imported_release_config_name}")
    set(sep "\n")
  endif()

  set(RunCMake_TEST_FAILED "${failed}" PARENT_SCOPE)
endfunction()
