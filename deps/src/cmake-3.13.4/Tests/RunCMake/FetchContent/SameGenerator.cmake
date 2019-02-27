include(FetchContent)

FetchContent_Declare(
  t1
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E echo "Download command executed"
)

FetchContent_Populate(t1)

file(STRINGS "${FETCHCONTENT_BASE_DIR}/t1-subbuild/CMakeCache.txt"
     matchLine REGEX "^CMAKE_GENERATOR:.*="
     LIMIT_COUNT 1
)
if(NOT matchLine MATCHES "${CMAKE_GENERATOR}")
  message(FATAL_ERROR "Generator line mismatch: ${matchLine}\n"
                      "  Expected type: ${CMAKE_GENERATOR}")
endif()
