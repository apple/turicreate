set(lfile "${CMAKE_CURRENT_BINARY_DIR}/file-to-lock")
set(ldir "${CMAKE_CURRENT_BINARY_DIR}/dir-to-lock")

message(STATUS "Simple lock")
file(LOCK ${lfile})

message(STATUS "Directory lock")
file(LOCK ${ldir} DIRECTORY)

message(STATUS "Release")
file(LOCK ${lfile} RELEASE)

function(foo)
  file(LOCK "${lfile}" GUARD FUNCTION)
endfunction()

message(STATUS "Lock function scope")
foo()

message(STATUS "Lock file scope")
add_subdirectory(subdir_test_unlock)

message(STATUS "Lock process scope")
file(LOCK "${lfile}" GUARD PROCESS)

message(STATUS "Error double lock")
file(LOCK "${lfile}" RESULT_VARIABLE lock_result)
if(lock_result STREQUAL "File already locked")
  message(STATUS "Ok")
else()
  message(STATUS FATAL_ERROR "Expected error message")
endif()

message(STATUS "Timeout 0")
file(LOCK "${lfile}" RELEASE)
file(LOCK "${lfile}" TIMEOUT 0)

message(STATUS "Timeout not 0")
file(LOCK "${lfile}" RELEASE)
file(LOCK "${lfile}" TIMEOUT 3)
