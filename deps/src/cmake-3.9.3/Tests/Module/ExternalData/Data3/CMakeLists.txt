set(Store0 ${CMAKE_BINARY_DIR}/ExternalData/Other)
set(Store1 ${CMAKE_BINARY_DIR}/ExternalData/Objects)
set(ExternalData_OBJECT_STORES ${Store0} ${Store1})
ExternalData_Add_Test(Data3
  NAME Data3Check
  COMMAND ${CMAKE_COMMAND}
    -D Data=DATA{Data.dat}
    -D Other=DATA{Other.dat}
    -D Store0=${Store0}
    -D Store1=${Store1}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/Data3Check.cmake
  )
ExternalData_Add_Target(Data3)
add_dependencies(Data3 Data1 Data2)
