project(random)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

add_library(timer OBJECT
  timer.cpp
)
add_dependencies(timer boost)