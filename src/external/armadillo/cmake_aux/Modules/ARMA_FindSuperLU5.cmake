# Try to find SuperLU 5.x
# Once done, this will define
#
#  SuperLU_FOUND        - system has SuperLU
#  SuperLU_LIBRARY      - Link this to use SuperLU
#  SuperLU_INCLUDE_DIR  - directory of SuperLU headers

find_path(SuperLU_INCLUDE_DIR slu_ddefs.h
  /usr/include/superlu/
  /usr/include/SuperLU/
  /usr/include/
  /usr/local/include/superlu/
  /usr/local/include/SuperLU/
  /usr/local/include/
  /opt/local/include/superlu/
  /opt/local/include/SuperLU/
  /opt/local/include/
)

find_library(SuperLU_LIBRARY
  NAMES superlu
  PATHS ${CMAKE_SYSTEM_LIBRARY_PATH} /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /opt/local/lib64 /opt/local/lib
)

set(SuperLU_FOUND NO)

if(NOT SuperLU_FIND_QUIETLY)
  message(STATUS "Looking for SuperLU version 5")
endif()

if(SuperLU_LIBRARY AND SuperLU_INCLUDE_DIR)

  # Now determine if the version is version 5.
  # This voodoo is required as SuperLU developers like to keep things messy.

  if(EXISTS "${SuperLU_INCLUDE_DIR}/slu_ddefs.h")

    file(READ "${SuperLU_INCLUDE_DIR}/slu_ddefs.h" _slu_HEADER_CONTENTS)

    string(REGEX REPLACE ".*version ([0-9]).*" "\\1" SLU_VERSION_MAJOR "${_slu_HEADER_CONTENTS}")

    if("${SLU_VERSION_MAJOR}" EQUAL "5")
      set(SuperLU_FOUND YES)
    else()
      if("${SLU_VERSION_MAJOR}" EQUAL "4")

        # NOTE: SLU_VERSION_MAJOR = 4 is not 100% reliable, as the version string in slu_ddefs.h wasn't updated in SuperLU 5.0, 5.1, 5.2
        # NOTE: We need to explicitly check the prototype of dgssvx() to determine whether this is version 4 or 5.

        string(FIND "${_slu_HEADER_CONTENTS}" "dgssvx" _slu_dgssvx_start)

        if("${_slu_dgssvx_start}" GREATER 0)

          string(SUBSTRING "${_slu_HEADER_CONTENTS}" ${_slu_dgssvx_start} -1 _slu_dgssvx_tmp)

          string(FIND "${_slu_dgssvx_tmp}" ";" _slu_dgssvx_len)

          string(SUBSTRING "${_slu_HEADER_CONTENTS}" ${_slu_dgssvx_start} ${_slu_dgssvx_len} _slu_dgssvx_proto)

          string(FIND "${_slu_dgssvx_proto}" "GlobalLU_t" _slu_dgssvx_check)

          if("${_slu_dgssvx_check}" EQUAL -1)
            # in version 4, dgssvx() doesn't have a GlobalLU_t argument
            if(NOT SuperLU_FIND_QUIETLY)
              message(STATUS "Found SuperLU, but it doesn't appear to be version 5")
            endif()
          else()
            set(SuperLU_FOUND YES)
          endif()
        endif()
      else()
        if(NOT SuperLU_FIND_QUIETLY)
          message(STATUS "Found SuperLU version ${SLU_VERSION_MAJOR}, but version 5 is required")
        endif()
      endif()
    endif()
  else()
    if(NOT SuperLU_FIND_QUIETLY)
      message(STATUS "Found SuperLU and headers, but could not verify version 5")
    endif()
  endif()
endif()

if(SuperLU_LIBRARY AND NOT SuperLU_INCLUDE_DIR)
  if(NOT SuperLU_FIND_QUIETLY)
    message(STATUS "Incomplete installation of SuperLU: no SuperLU headers found")
  endif()
endif()

if(NOT SuperLU_FOUND)
  if(NOT SuperLU_FIND_QUIETLY)
    message(STATUS "Could not find SuperLU")
  endif()
else()
  message(STATUS "Found SuperLU: ${SuperLU_LIBRARY}")
endif()
