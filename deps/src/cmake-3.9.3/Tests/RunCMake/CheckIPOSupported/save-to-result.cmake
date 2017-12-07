project(${RunCMake_TEST} LANGUAGES C)
check_ipo_supported(RESULT result OUTPUT output)

string(COMPARE EQUAL "${result}" "" is_empty)
if(is_empty)
  message(FATAL_ERROR "Result variable is empty")
endif()

string(COMPARE EQUAL "${result}" "YES" is_yes)
string(COMPARE EQUAL "${result}" "NO" is_no)

if(is_yes)
  # Compiler supports IPO
elseif(is_no)
  # Compiler doesn't support IPO, output should not be empty.
  string(COMPARE EQUAL "${output}" "" is_empty)
  if(is_empty)
    message(FATAL_ERROR "Output is empty")
  endif()
else()
  message(FATAL_ERROR "Unexpected result: ${result}")
endif()
