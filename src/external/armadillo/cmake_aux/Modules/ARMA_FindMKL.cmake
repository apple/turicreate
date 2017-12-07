# - Find the MKL libraries (no includes)
# This module defines
#  MKL_LIBRARIES, the libraries needed to use Intel's implementation of BLAS & LAPACK.
#  MKL_FOUND, If false, do not try to use MKL.

## the link below explains why we're linking only with mkl_rt
## https://software.intel.com/en-us/articles/a-new-linking-model-single-dynamic-library-mkl_rt-since-intel-mkl-103

set(MKL_NAMES ${MKL_NAMES} mkl_rt)
#set(MKL_NAMES ${MKL_NAMES} mkl_lapack)
#set(MKL_NAMES ${MKL_NAMES} mkl_intel_thread)
#set(MKL_NAMES ${MKL_NAMES} mkl_core)
#set(MKL_NAMES ${MKL_NAMES} guide)
#set(MKL_NAMES ${MKL_NAMES} mkl)
#set(MKL_NAMES ${MKL_NAMES} iomp5)
#set(MKL_NAMES ${MKL_NAMES} pthread)


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  #set(MKL_NAMES ${MKL_NAMES} mkl_intel_lp64)
  set(MKL_ARCH intel64)
else()
  #set(MKL_NAMES ${MKL_NAMES} mkl_intel)
  set(MKL_ARCH ia32)
endif()

set(MKL_ROOT $ENV{MKLROOT} CACHE TYPE STRING)

if(NOT MKL_ROOT)
  set(MKL_ROOT "/opt/intel/mkl")
endif()


foreach (MKL_NAME ${MKL_NAMES})
  find_library(${MKL_NAME}_LIBRARY
    NAMES ${MKL_NAME}
    PATHS
    ${CMAKE_SYSTEM_LIBRARY_PATH}
    ${MKL_ROOT}/lib/${MKL_ARCH}
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/intel/composerxe/lib/intel64
    /opt/intel/composerxe/lib/ia32
    /opt/intel/composerxe/lib/mkl/lib/intel64
    /opt/intel/composerxe/lib/mkl/lib/ia32
    /usr/local/intel/composerxe/lib/intel64
    /usr/local/intel/composerxe/lib/ia32
    /usr/local/intel/composerxe/lib/mkl/lib/intel64
    /usr/local/intel/composerxe/lib/mkl/lib/ia32
    /opt/intel/lib
    /opt/intel/lib/intel64
    /opt/intel/lib/em64t
    /opt/intel/lib/lib64
    /opt/intel/lib/ia32
    /opt/intel/mkl/lib
    /opt/intel/mkl/lib/intel64
    /opt/intel/mkl/lib/em64t
    /opt/intel/mkl/lib/lib64
    /opt/intel/mkl/lib/ia32
    /opt/intel/mkl/*/lib
    /opt/intel/mkl/*/lib/intel64
    /opt/intel/mkl/*/lib/em64t
    /opt/intel/mkl/*/lib/lib64
    /opt/intel/mkl/*/lib/32
    /opt/intel/*/mkl/lib
    /opt/intel/*/mkl/lib/intel64
    /opt/intel/*/mkl/lib/em64t
    /opt/intel/*/mkl/lib/lib64
    /opt/intel/*/mkl/lib/ia32
    /opt/mkl/lib
    /opt/mkl/lib/intel64
    /opt/mkl/lib/em64t
    /opt/mkl/lib/lib64
    /opt/mkl/lib/ia32
    /opt/mkl/*/lib
    /opt/mkl/*/lib/intel64
    /opt/mkl/*/lib/em64t
    /opt/mkl/*/lib/lib64
    /opt/mkl/*/lib/32
    /usr/local/intel/lib
    /usr/local/intel/lib/intel64
    /usr/local/intel/lib/em64t
    /usr/local/intel/lib/lib64
    /usr/local/intel/lib/ia32
    /usr/local/intel/mkl/lib
    /usr/local/intel/mkl/lib/intel64
    /usr/local/intel/mkl/lib/em64t
    /usr/local/intel/mkl/lib/lib64
    /usr/local/intel/mkl/lib/ia32
    /usr/local/intel/mkl/*/lib
    /usr/local/intel/mkl/*/lib/intel64
    /usr/local/intel/mkl/*/lib/em64t
    /usr/local/intel/mkl/*/lib/lib64
    /usr/local/intel/mkl/*/lib/32
    /usr/local/intel/*/mkl/lib
    /usr/local/intel/*/mkl/lib/intel64
    /usr/local/intel/*/mkl/lib/em64t
    /usr/local/intel/*/mkl/lib/lib64
    /usr/local/intel/*/mkl/lib/ia32
    /usr/local/mkl/lib
    /usr/local/mkl/lib/intel64
    /usr/local/mkl/lib/em64t
    /usr/local/mkl/lib/lib64
    /usr/local/mkl/lib/ia32
    /usr/local/mkl/*/lib
    /usr/local/mkl/*/lib/intel64
    /usr/local/mkl/*/lib/em64t
    /usr/local/mkl/*/lib/lib64
    /usr/local/mkl/*/lib/32
    )

  set(TMP_LIBRARY ${${MKL_NAME}_LIBRARY})

  if(TMP_LIBRARY)
    set(MKL_LIBRARIES ${MKL_LIBRARIES} ${TMP_LIBRARY})
  endif()
endforeach()

if(MKL_LIBRARIES)
  set(MKL_FOUND "YES")
else()
  set(MKL_FOUND "NO")
endif()

if(MKL_FOUND)
  if(NOT MKL_FIND_QUIETLY)
    message(STATUS "Found MKL libraries: ${MKL_LIBRARIES}")
  endif()
else()
  if(MKL_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find MKL libraries")
  endif()
endif()

# mark_as_advanced(MKL_LIBRARY)
