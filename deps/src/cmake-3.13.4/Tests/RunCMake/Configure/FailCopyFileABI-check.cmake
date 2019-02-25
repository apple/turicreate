set(log "${RunCMake_TEST_BINARY_DIR}/CMakeFiles/CMakeError.log")
if(EXISTS "${log}")
  file(READ "${log}" error_log)
else()
  set(error_log "")
endif()
string(REPLACE "\r\n" "\n" regex "Cannot copy output executable.*
to destination specified by COPY_FILE:.*
Unable to find the executable at any of:
  .*\\.missing")
if(NOT error_log MATCHES "${regex}")
  string(REGEX REPLACE "\n" "\n  " error_log "  ${error_log}")
  set(RunCMake_TEST_FAILED "Log file:\n ${log}\ndoes not have expected COPY_FILE failure message:\n${error_log}")
endif()
