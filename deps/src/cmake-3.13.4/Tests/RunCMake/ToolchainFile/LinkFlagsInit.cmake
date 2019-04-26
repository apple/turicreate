set(CMAKE_C_COMPILER_FORCED 1) # skip compiler test so we can check cached values
enable_language(C)
foreach(t EXE SHARED MODULE STATIC)
  foreach(c "" _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
    message(STATUS "CMAKE_${t}_LINKER_FLAGS${c}='${CMAKE_${t}_LINKER_FLAGS${c}}'")
  endforeach()
endforeach()
