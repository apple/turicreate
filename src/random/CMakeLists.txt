project(random)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(WIN32)
  SET(PLATFORM_DEPENDENCIES crypt32)
endif()

make_library(random OBJECT
  SOURCES
    random.cpp
    alias.cpp
  REQUIRES
    boost
    logger
    parallel
    util
    ${PLATFORM_DEPENDENCIES}
  EXTERNAL_VISIBILITY
)
