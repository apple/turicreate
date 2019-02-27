set(SUBDIR_DEFINED 1 PARENT_SCOPE)
set(SUBDIR_UNDEFINED PARENT_SCOPE)

# The above sets should not affect the current scope.
if(DEFINED SUBDIR_UNDEFINED)
  PASS("SubdirScope Undefine Test" "(${SUBDIR_UNDEFINED})")
else()
  FAILED("SubdirScope Undefine Test" "(${SUBDIR_UNDEFINED})")
endif()
if(DEFINED SUBDIR_DEFINED)
  FAILED("SubdirScope Define Test" "(${SUBDIR_DEFINED})")
else()
  PASS("SubdirScope Define Test" "(${SUBDIR_DEFINED})")
endif()
