if("a
b" STREQUAL "a\nb")
  message("CRLF->LF worked")
else()
  message(FATAL_ERROR "CRLF->LF failed")
endif()
