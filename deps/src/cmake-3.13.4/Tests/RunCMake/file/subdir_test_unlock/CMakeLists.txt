message(STATUS "Lock in subdirectory")
file(LOCK "${lfile}" GUARD FILE)
