foreach(f
    "${testLib1}.genex"
    "${prefix}/doc/testLib1file1.txt"
    "${prefix}/doc/testLib1file2.txt"
    )
  if(EXISTS "${f}")
    message(STATUS "'${f}' exists!")
  else()
    message(FATAL_ERROR "Missing file:\n ${f}")
  endif()
endforeach()
