function(BuildTargetInSubProject P T E)
  try_compile(RESULTVAR
    ${CMAKE_CURRENT_BINARY_DIR}/subproject
    ${CMAKE_CURRENT_SOURCE_DIR}/subproject
    ${P} ${T} OUTPUT_VARIABLE O)
  if(E AND RESULTVAR)
    message(STATUS "${P} target ${T} succeeded as expected")
  elseif(E AND NOT RESULTVAR)
    message(FATAL_ERROR "${P} target ${T} failed but should have succeeded.  Output:${O}")
  elseif(NOT E AND NOT RESULTVAR)
    message(STATUS "${P} target ${T} failed as expected")
  elseif(NOT E AND RESULTVAR)
    message(FATAL_ERROR "${P} target ${T} succeeded but should have failed.  Output:${O}")
  endif()
endfunction()
