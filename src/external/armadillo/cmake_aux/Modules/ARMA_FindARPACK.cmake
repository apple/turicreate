# - Try to find ARPACK
# Once done this will define
#
#  ARPACK_FOUND        - system has ARPACK
#  ARPACK_LIBRARY      - Link this to use ARPACK


find_library(ARPACK_LIBRARY
  NAMES arpack
  PATHS ${CMAKE_SYSTEM_LIBRARY_PATH} /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib
  )


IF (ARPACK_LIBRARY)
  SET(ARPACK_FOUND YES)
ELSE ()
  SET(ARPACK_FOUND NO)
ENDIF ()


IF (ARPACK_FOUND)
  IF (NOT ARPACK_FIND_QUIETLY)
     MESSAGE(STATUS "Found ARPACK: ${ARPACK_LIBRARY}")
  ENDIF (NOT ARPACK_FIND_QUIETLY)
ELSE (ARPACK_FOUND)
  IF (ARPACK_FIND_REQUIRED)
     MESSAGE(FATAL_ERROR "Could not find ARPACK")
  ENDIF (ARPACK_FIND_REQUIRED)
ENDIF (ARPACK_FOUND)
