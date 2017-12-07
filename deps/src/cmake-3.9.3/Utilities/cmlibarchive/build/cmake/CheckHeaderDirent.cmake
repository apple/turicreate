# - Check if the system has the specified type
# CHECK_HEADER_DIRENT (HEADER1 HEARDER2 ...)
#
#  HEADER - the header(s) where the prototype should be declared
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
# Copyright (c) 2009, Michihiro NAKAJIMA
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


INCLUDE(CheckTypeExists)

MACRO (CHECK_HEADER_DIRENT)
  CHECK_TYPE_EXISTS("DIR *" dirent.h     HAVE_DIRENT_H)
  IF(NOT HAVE_DIRENT_H)
    CHECK_TYPE_EXISTS("DIR *" sys/ndir.h  HAVE_SYS_NDIR_H)
    IF(NOT HAVE_SYS_NDIR_H)
      CHECK_TYPE_EXISTS("DIR *" ndir.h      HAVE_NDIR_H)
      IF(NOT HAVE_NDIR_H)
        CHECK_TYPE_EXISTS("DIR *" sys/dir.h   HAVE_SYS_DIR_H)
      ENDIF(NOT HAVE_NDIR_H)
    ENDIF(NOT HAVE_SYS_NDIR_H)
  ENDIF(NOT HAVE_DIRENT_H)
ENDMACRO (CHECK_HEADER_DIRENT)

