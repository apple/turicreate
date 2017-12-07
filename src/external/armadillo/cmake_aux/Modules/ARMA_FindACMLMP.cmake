# - Find AMD's ACMLMP library (no includes) which provides optimised and parallelised BLAS and LAPACK functions
# This module defines
#  ACMLMP_LIBRARIES, the libraries needed to use ACMLMP.
#  ACMLMP_FOUND, If false, do not try to use ACMLMP.
# also defined, but not for general use are
#  ACMLMP_LIBRARY, where to find the ACMLMP library.

SET(ACMLMP_NAMES ${ACMLMP_NAMES} acml_mp)
FIND_LIBRARY(ACMLMP_LIBRARY
  NAMES ${ACMLMP_NAMES}
  PATHS /usr/lib64 /usr/lib /usr/*/lib64 /usr/*/lib /usr/*/gfortran64_mp/lib/ /usr/*/gfortran32_mp/lib/ /usr/local/lib64 /usr/local/lib /opt/lib64 /opt/lib /opt/*/lib64 /opt/*/lib /opt/*/gfortran64_mp/lib/ /opt/*/gfortran32_mp/lib/
  )

IF (ACMLMP_LIBRARY)
  SET(ACMLMP_LIBRARIES ${ACMLMP_LIBRARY})
  SET(ACMLMP_FOUND "YES")
ELSE (ACMLMP_LIBRARY)
  SET(ACMLMP_FOUND "NO")
ENDIF (ACMLMP_LIBRARY)


IF (ACMLMP_FOUND)
   IF (NOT ACMLMP_FIND_QUIETLY)
      MESSAGE(STATUS "Found ACMLMP: ${ACMLMP_LIBRARIES}")
   ENDIF (NOT ACMLMP_FIND_QUIETLY)
ELSE (ACMLMP_FOUND)
   IF (ACMLMP_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find ACMLMP")
   ENDIF (ACMLMP_FIND_REQUIRED)
ENDIF (ACMLMP_FOUND)

# Deprecated declarations.
GET_FILENAME_COMPONENT (NATIVE_ACMLMP_LIB_PATH ${ACMLMP_LIBRARY} PATH)

MARK_AS_ADVANCED(
  ACMLMP_LIBRARY
  )
