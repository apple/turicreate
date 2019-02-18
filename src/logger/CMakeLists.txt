project(Turi)

#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/

make_library(logger OBJECT
  SOURCES
    error.cpp
    logger.cpp
    backtrace.cpp
    log_rotate.cpp
    log_level_setter.cpp
  REQUIRES
    boost
    timer
    parallel
    stack_trace
    cppipc
  EXTERNAL_VISIBILITY
)
