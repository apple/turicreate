# - Find libgcc
# Find the libgcc library.
#
#  LIBGCC_LIBRARIES      - List of libraries when using libgcc
#  LIBGCC_FOUND          - True if libgcc found.

IF (LIBGCC_LIBRARY)
  # Already in cache, be silent
  SET(LIBGCC_FIND_QUIETLY TRUE)
ENDIF (LIBGCC_LIBRARY)

FIND_LIBRARY(LIBGCC_LIBRARY NAMES gcc libgcc)

# handle the QUIETLY and REQUIRED arguments and set LIBGCC_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBGCC DEFAULT_MSG LIBGCC_LIBRARY)

IF(LIBGCC_FOUND)
  SET(LIBGCC_LIBRARIES ${LIBGCC_LIBRARY})
  SET(HAVE_LIBGCC 1)
ENDIF(LIBGCC_FOUND)
