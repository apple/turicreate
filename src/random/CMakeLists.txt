project(random)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(WIN32)
  SET(PLATFORM_DEPENDENCIES crypt32)
endif()

add_library(random OBJECT
  random.cpp
  alias.cpp
)
add_dependencies(random boost)