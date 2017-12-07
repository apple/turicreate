project(Turi)

#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/

make_library(logger
  SOURCES
    logger.cpp
    backtrace.cpp
    log_rotate.cpp
    log_level_setter.cpp
  REQUIRES
    boost
    timer
    parallel
    EXTERNAL_VISIBILITY
  )
