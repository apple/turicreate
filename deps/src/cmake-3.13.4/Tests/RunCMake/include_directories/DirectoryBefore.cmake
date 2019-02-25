include_directories(AfterDir)
include_directories(BEFORE BeforeDir)
get_property(dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "INCLUDE_DIRECTORIES: '${dirs}'")
