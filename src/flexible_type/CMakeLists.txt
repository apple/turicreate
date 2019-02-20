project(flexible_type)

#/**************************************************************************/
#/*                                                                        */
#/*                             Logger Library                             */
#/*                                                                        */
#/**************************************************************************/

make_library(flexible_type OBJECT
  SOURCES
    flexible_type.cpp
    string_escape.cpp
    flexible_type_conversion_utilities.cpp
    flexible_type_spirit_parser.cpp
  REQUIRES
    logger
    image_type
  EXTERNAL_VISIBILITY
)
