# - Find Nettle
# Find the Nettle include directory and library
#
#  NETTLE_INCLUDE_DIR    - where to find <nettle/sha.h>, etc.
#  NETTLE_LIBRARIES      - List of libraries when using libnettle.
#  NETTLE_FOUND          - True if libnettle found.

IF (NETTLE_INCLUDE_DIR)
  # Already in cache, be silent
  SET(NETTLE_FIND_QUIETLY TRUE)
ENDIF (NETTLE_INCLUDE_DIR)

FIND_PATH(NETTLE_INCLUDE_DIR nettle/md5.h nettle/ripemd160.h nettle/sha.h)
FIND_LIBRARY(NETTLE_LIBRARY NAMES nettle libnettle)

# handle the QUIETLY and REQUIRED arguments and set NETTLE_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NETTLE DEFAULT_MSG NETTLE_LIBRARY NETTLE_INCLUDE_DIR)

IF(NETTLE_FOUND)
  SET(NETTLE_LIBRARIES ${NETTLE_LIBRARY})
ENDIF(NETTLE_FOUND)
