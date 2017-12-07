set(OpenBLAS_NAMES)
set(OpenBLAS_NAMES ${OpenBLAS_NAMES} openblaso)
set(OpenBLAS_NAMES ${OpenBLAS_NAMES} openblasp)
set(OpenBLAS_NAMES ${OpenBLAS_NAMES} openblas )

set(OpenBLAS_TMP_LIBRARY)
set(OpenBLAS_TMP_LIBRARIES)


foreach (OpenBLAS_NAME ${OpenBLAS_NAMES})
  find_library(${OpenBLAS_NAME}_LIBRARY
    NAMES ${OpenBLAS_NAME}
    PATHS ${CMAKE_SYSTEM_LIBRARY_PATH} /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib
    )

  set(OpenBLAS_TMP_LIBRARY ${${OpenBLAS_NAME}_LIBRARY})

  if(OpenBLAS_TMP_LIBRARY)
    set(OpenBLAS_TMP_LIBRARIES ${OpenBLAS_TMP_LIBRARIES} ${OpenBLAS_TMP_LIBRARY})
  endif()
endforeach()


# use only one library

if(OpenBLAS_TMP_LIBRARIES)
  list(GET OpenBLAS_TMP_LIBRARIES 0 OpenBLAS_LIBRARY)
endif()


if(OpenBLAS_LIBRARY)
  set(OpenBLAS_LIBRARIES ${OpenBLAS_LIBRARY})
  set(OpenBLAS_FOUND "YES")
else()
  set(OpenBLAS_FOUND "NO")
endif()


if(OpenBLAS_FOUND)
  if (NOT OpenBLAS_FIND_QUIETLY)
    message(STATUS "Found OpenBLAS: ${OpenBLAS_LIBRARIES}")
  endif()
else()
  if(OpenBLAS_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find OpenBLAS")
  endif()
endif()


# mark_as_advanced(OpenBLAS_LIBRARY)
