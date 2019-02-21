set(ExternalData_NO_SYMLINKS 1)
ExternalData_Add_Test(DataNoSymlinks
  NAME DataNoSymlinksCheck
  COMMAND ${CMAKE_COMMAND}
    -D Data=DATA{Data.dat}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/DataNoSymlinksCheck.cmake
  )
ExternalData_Add_Target(DataNoSymlinks)
