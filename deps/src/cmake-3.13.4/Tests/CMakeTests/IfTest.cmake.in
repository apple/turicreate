# Prepare variable definitions.
set(VAR_UNDEFINED)
set(VAR_PATH /some/path/to/a/file.txt)
set(FALSE_NAMES OFF NO FALSE N FOO-NOTFOUND IGNORE Off No False Ignore off n no false ignore)
set(TRUE_NAMES ON YES TRUE Y On Yes True on yes true y)
foreach(_arg "" 0 1 2 ${TRUE_NAMES} ${FALSE_NAMES})
  set(VAR_${_arg} "${_arg}")
endforeach()

macro(test_vars _old)
  # Variables set to false or not set.
  foreach(_var "" 0 ${FALSE_NAMES} UNDEFINED)
    if(VAR_${_var})
      message(FATAL_ERROR "${_old}if(VAR_${_var}) is true!")
    else()
      message(STATUS "${_old}if(VAR_${_var}) is false")
    endif()

    if(NOT VAR_${_var})
      message(STATUS "${_old}if(NOT VAR_${_var}) is true")
    else()
      message(FATAL_ERROR "${_old}if(NOT VAR_${_var}) is false!")
    endif()
  endforeach()

  # Variables set to true.
  foreach(_var 1 2 ${TRUE_NAMES} PATH)
    if(VAR_${_var})
      message(STATUS "${_old}if(VAR_${_var}) is true")
    else()
      message(FATAL_ERROR "${_old}if(VAR_${_var}) is false!")
    endif()

    if(NOT VAR_${_var})
      message(FATAL_ERROR "${_old}if(NOT VAR_${_var}) is true!")
    else()
      message(STATUS "${_old}if(NOT VAR_${_var}) is false")
    endif()
  endforeach()
endmacro()

#-----------------------------------------------------------------------------
# Test the OLD behavior of CMP0012.
cmake_policy(SET CMP0012 OLD)

# False constants not recognized (still false).
foreach(_false "" ${FALSE_NAMES})
  if("${_false}")
    message(FATAL_ERROR "OLD if(${_false}) is true!")
  else()
    message(STATUS "OLD if(${_false}) is false")
  endif()

  if(NOT "${_false}")
    message(STATUS "OLD if(NOT ${_false}) is true")
  else()
    message(FATAL_ERROR "OLD if(NOT ${_false}) is false!")
  endif()
endforeach()

# True constants not recognized.
foreach(_false ${TRUE_NAMES})
  if(${_false})
    message(FATAL_ERROR "OLD if(${_false}) is true!")
  else()
    message(STATUS "OLD if(${_false}) is false")
  endif()

  if(NOT ${_false})
    message(STATUS "OLD if(NOT ${_false}) is true")
  else()
    message(FATAL_ERROR "OLD if(NOT ${_false}) is false!")
  endif()
endforeach()

# Numbers not recognized properly.
foreach(_num 2 -2 2.0 -2.0 2x -2x)
  if(${_num})
    message(FATAL_ERROR "OLD if(${_num}) is true!")
  else()
    message(STATUS "OLD if(${_num}) is false")
  endif()

  if(NOT ${_num})
    message(FATAL_ERROR "OLD if(NOT ${_num}) is true!")
  else()
    message(STATUS "OLD if(NOT ${_num}) is false")
  endif()
endforeach()

test_vars("OLD ")

#-----------------------------------------------------------------------------

# Test the NEW behavior of CMP0012.
cmake_policy(SET CMP0012 NEW)

# Test false constants.
foreach(_false "" 0 ${FALSE_NAMES})
  if("${_false}")
    message(FATAL_ERROR "if(${_false}) is true!")
  else()
    message(STATUS "if(${_false}) is false")
  endif()

  if(NOT "${_false}")
    message(STATUS "if(NOT ${_false}) is true")
  else()
    message(FATAL_ERROR "if(NOT ${_false}) is false!")
  endif()
endforeach()

# Test true constants.
foreach(_true 1 ${TRUE_NAMES})
  if(${_true})
    message(STATUS "if(${_true}) is true")
  else()
    message(FATAL_ERROR "if(${_true}) is false!")
  endif()

  if(NOT ${_true})
    message(FATAL_ERROR "if(NOT ${_true}) is true!")
  else()
    message(STATUS "if(NOT ${_true}) is false")
  endif()
endforeach()

# Numbers recognized properly.
foreach(_num 2 -2 2.0 -2.0)
  if(${_num})
    message(STATUS "if(${_num}) is true")
  else()
    message(FATAL_ERROR "if(${_num}) is false!")
  endif()

  if(NOT ${_num})
    message(FATAL_ERROR "if(NOT ${_num}) is true!")
  else()
    message(STATUS "if(NOT ${_num}) is false")
  endif()
endforeach()

# Bad numbers not recognized.
foreach(_bad 2x -2x)
  if(${_bad})
    message(FATAL_ERROR "if(${_bad}) is true!")
  else()
    message(STATUS "if(${_bad}) is false")
  endif()

  if(NOT ${_bad})
    message(STATUS "if(NOT ${_bad}) is true")
  else()
    message(FATAL_ERROR "if(NOT ${_bad}) is false!")
  endif()
endforeach()

test_vars("")
