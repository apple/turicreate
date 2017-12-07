# - Check if the system has the specified type
# CHECK_TYPE_EXISTS (TYPE HEADER VARIABLE)
#
#  TYPE - the name of the type or struct or class you are interested in
#  HEADER - the header(s) where the prototype should be declared
#  VARIABLE - variable to store the result
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
# Copyright (c) 2009, Michihiro NAKAJIMA
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


INCLUDE(CheckCSourceCompiles)

MACRO (CHECK_TYPE_EXISTS _TYPE _HEADER _RESULT)
   SET(_INCLUDE_FILES)
   FOREACH (it ${_HEADER})
      SET(_INCLUDE_FILES "${_INCLUDE_FILES}#include <${it}>\n")
   ENDFOREACH (it)

   SET(_CHECK_TYPE_EXISTS_SOURCE_CODE "
${_INCLUDE_FILES}
int main()
{
   static ${_TYPE} tmp;
   if (sizeof(tmp))
      return 0;
  return 0;
}
")
   CHECK_C_SOURCE_COMPILES("${_CHECK_TYPE_EXISTS_SOURCE_CODE}" ${_RESULT})

ENDMACRO (CHECK_TYPE_EXISTS)

