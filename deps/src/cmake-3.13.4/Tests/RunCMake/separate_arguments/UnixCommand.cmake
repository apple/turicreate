set(unix_cmd "a \"b c\" 'd e' \";\" \\ \\'\\\" '\\'' \"\\\"\"")
set(unix_exp "a;b c;d e;\;; '\";';\"")
separate_arguments(unix_out UNIX_COMMAND "${unix_cmd}")

if(NOT "${unix_out}" STREQUAL "${unix_exp}")
  message(FATAL_ERROR "separate_arguments unix-style failed.  "
    "Expected\n  [${unix_exp}]\nbut got\n  [${unix_out}]\n")
endif()
