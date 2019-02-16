project(random)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

make_library(timer OBJECT
  SOURCES
    timer.cpp
  REQUIRES
    parallel
    boost
  EXTERNAL_VISIBILITY
)
