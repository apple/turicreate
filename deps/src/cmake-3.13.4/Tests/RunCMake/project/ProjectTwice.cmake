cmake_policy(SET CMP0048 NEW)
project(ProjectTwiceTestFirst
  VERSION 1.2.3.4
  DESCRIPTION "Test Project"
  HOMEPAGE_URL "http://example.com"
  LANGUAGES NONE
)

project(ProjectTwiceTestSecond LANGUAGES NONE)

foreach(var
  PROJECT_VERSION
  PROJECT_VERSION_MAJOR
  PROJECT_VERSION_MINOR
  PROJECT_VERSION_PATCH
  PROJECT_VERSION_TWEAK
  PROJECT_DESCRIPTION
  PROJECT_HOMEPAGE_URL
)
  if(${var})
    message(SEND_ERROR "${var} set but should be empty")
  endif()
  if(CMAKE_${var})
    message(SEND_ERROR "CMAKE_${var} set but should be empty")
  endif()
endforeach()
