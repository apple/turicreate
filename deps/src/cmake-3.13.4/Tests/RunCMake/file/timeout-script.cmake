if(NOT file_to_lock)
  message(FATAL_ERROR "file_to_lock is empty")
endif()

file(LOCK "${file_to_lock}" TIMEOUT 1)
