project(flexible_type)

#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/

add_library(flexible_type OBJECT
  flexible_type.cpp
  string_escape.cpp
  flexible_type_conversion_utilities.cpp
  flexible_type_spirit_parser.cpp
)
add_dependencies(flexible_type boost)