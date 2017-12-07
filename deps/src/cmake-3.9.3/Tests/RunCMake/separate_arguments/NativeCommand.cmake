set(unix_cmd "a \"b c\" 'd e' \";\" \\ \\'\\\" '\\'' \"\\\"\"")
set(unix_exp "a;b c;d e;\;; '\";';\"")

set(windows_cmd "a \"b c\" 'd e' \";\" \\ \"c:\\windows\\path\\\\\" \\\"")
set(windows_exp "a;b c;'d;e';\;;\\;c:\\windows\\path\\;\"")

if(CMAKE_HOST_WIN32)
  set(native_cmd "${windows_cmd}")
  set(native_exp "${windows_exp}")
else()
  set(native_cmd "${unix_cmd}")
  set(native_exp "${unix_exp}")
endif()
separate_arguments(native_out NATIVE_COMMAND "${native_cmd}")

if(NOT "${native_out}" STREQUAL "${native_exp}")
  message(FATAL_ERROR "separate_arguments native-style failed.  "
    "Expected\n  [${native_exp}]\nbut got\n  [${native_out}]\n")
endif()
