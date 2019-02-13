project(Turi)

#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/

add_library(logger OBJECT
  error.cpp
  logger.cpp
  backtrace.cpp
  log_rotate.cpp
  log_level_setter.cpp
)
add_dependencies(logger boost)
