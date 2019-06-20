project(process)

if(WIN32)
  SET(PLATFORM_SOURCES process_win.cpp process_util_win.cpp)
else()
  SET(PLATFORM_SOURCES process_unix.cpp process_util_unix.cpp)
endif()

make_library(process OBJECT
  SOURCES
    process.cpp
    ${PLATFORM_SOURCES}
  REQUIRES
    boost
    util
    logger
)
