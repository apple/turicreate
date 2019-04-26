set(lock "${CMAKE_CURRENT_BINARY_DIR}/file-to-lock")

if(WIN32)
  string(TOLOWER ${lock} lock)
endif()

file(LOCK ${lock} TIMEOUT 0)
file(LOCK ${lock} RELEASE)

file(LOCK ${lock} TIMEOUT 0)
file(LOCK ${lock} RELEASE)
