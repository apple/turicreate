set(lfile "${CMAKE_CURRENT_BINARY_DIR}/file-to-lock")
FILE(WRITE "${lfile}" "")

# Try to lock file '${lfile}/cmake.lock'. Since `lfile` is not a directory
# expected that operation will fail.
file(LOCK "${lfile}" DIRECTORY)
