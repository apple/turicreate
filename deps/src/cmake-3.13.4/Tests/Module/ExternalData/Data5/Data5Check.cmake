file(STRINGS "${Data5}" lines LIMIT_INPUT 1024)
if(NOT "x${lines}" STREQUAL "xInput file already transformed.")
  message(SEND_ERROR "Input file:\n  ${Data5}\ndoes not have expected content, but [[${lines}]]")
endif()
