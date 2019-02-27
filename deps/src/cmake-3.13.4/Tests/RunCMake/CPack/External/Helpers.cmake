function(getPackageNameGlobexpr NAME COMPONENT VERSION REVISION FILE_NO RESULT_VAR)
  set(${RESULT_VAR} "${NAME}-${VERSION}-*.json" PARENT_SCOPE)
endfunction()

function(getPackageContentList FILE RESULT_VAR)
  set("${RESULT_VAR}" "" PARENT_SCOPE)
endfunction()

function(toExpectedContentList FILE_NO CONTENT_VAR)
  set("${CONTENT_VAR}" "" PARENT_SCOPE)
endfunction()

set(ALL_FILES_GLOB "*.json")

function(check_ext_json EXPECTED_FILE ACTUAL_FILE)
  file(READ "${EXPECTED_FILE}" _expected_regex)
  file(READ "${ACTUAL_FILE}" _actual_contents)

  string(REGEX REPLACE "\n+$" "" _expected_regex "${_expected_regex}")
  string(REGEX REPLACE "\n+$" "" _actual_contents "${_actual_contents}")

  if(NOT "${_actual_contents}" MATCHES "${_expected_regex}")
    message(FATAL_ERROR
      "Output JSON does not match expected regex.\n"
      "Expected regex:\n"
      "${_expected_regex}\n"
      "Actual output:\n"
      "${_actual_contents}\n"
    )
  endif()
endfunction()
