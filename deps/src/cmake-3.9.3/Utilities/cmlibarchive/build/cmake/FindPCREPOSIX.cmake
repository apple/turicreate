# - Find pcreposix
# Find the native PCRE and PCREPOSIX include and libraries
#
#  PCRE_INCLUDE_DIR    - where to find pcreposix.h, etc.
#  PCREPOSIX_LIBRARIES - List of libraries when using libpcreposix.
#  PCRE_LIBRARIES      - List of libraries when using libpcre.
#  PCREPOSIX_FOUND     - True if libpcreposix found.
#  PCRE_FOUND          - True if libpcre found.

IF (PCRE_INCLUDE_DIR)
  # Already in cache, be silent
  SET(PCRE_FIND_QUIETLY TRUE)
ENDIF (PCRE_INCLUDE_DIR)

FIND_PATH(PCRE_INCLUDE_DIR pcreposix.h)
FIND_LIBRARY(PCREPOSIX_LIBRARY NAMES pcreposix libpcreposix)
FIND_LIBRARY(PCRE_LIBRARY NAMES pcre libpcre)

# handle the QUIETLY and REQUIRED arguments and set PCREPOSIX_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCREPOSIX DEFAULT_MSG PCREPOSIX_LIBRARY PCRE_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE DEFAULT_MSG PCRE_LIBRARY)

IF(PCREPOSIX_FOUND)
  SET(PCREPOSIX_LIBRARIES ${PCREPOSIX_LIBRARY})
  SET(HAVE_LIBPCREPOSIX 1)
  SET(HAVE_PCREPOSIX_H 1)
ENDIF(PCREPOSIX_FOUND)

IF(PCRE_FOUND)
  SET(PCRE_LIBRARIES ${PCRE_LIBRARY})
  SET(HAVE_LIBPCRE 1)
ENDIF(PCRE_FOUND)
