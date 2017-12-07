# - Find a BLAS library (no includes)
# This module defines
#  BLAS_LIBRARIES, the libraries needed to use BLAS.
#  BLAS_FOUND, If false, do not try to use BLAS.
# also defined, but not for general use are
#  BLAS_LIBRARY, where to find the BLAS library.

SET(BLAS_NAMES ${BLAS_NAMES} blas)
FIND_LIBRARY(BLAS_LIBRARY
  NAMES ${BLAS_NAMES}
  PATHS /usr/lib64/atlas /usr/lib/atlas /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib
  )

IF (BLAS_LIBRARY)
  SET(BLAS_LIBRARIES ${BLAS_LIBRARY})
  SET(BLAS_FOUND "YES")
ELSE (BLAS_LIBRARY)
  SET(BLAS_FOUND "NO")
ENDIF (BLAS_LIBRARY)


IF (BLAS_FOUND)
   IF (NOT BLAS_FIND_QUIETLY)
      MESSAGE(STATUS "Found BLAS: ${BLAS_LIBRARIES}")
   ENDIF (NOT BLAS_FIND_QUIETLY)
ELSE (BLAS_FOUND)
   IF (BLAS_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find BLAS")
   ENDIF (BLAS_FIND_REQUIRED)
ENDIF (BLAS_FOUND)

# Deprecated declarations.
GET_FILENAME_COMPONENT (NATIVE_BLAS_LIB_PATH ${BLAS_LIBRARY} PATH)

MARK_AS_ADVANCED(
  BLAS_LIBRARY
  )
