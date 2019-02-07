project(process)

if(WIN32)
  SET(PLATFORM_SOURCES process_win.cpp process_util_win.cpp)
else()
  SET(PLATFORM_SOURCES process_unix.cpp process_util_unix.cpp)
endif()

add_library(process OBJECT
  process.cpp
  ${PLATFORM_SOURCES}
)
add_dependencies(process boost)