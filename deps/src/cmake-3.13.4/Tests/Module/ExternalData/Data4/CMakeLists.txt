set(Store0 ${CMAKE_BINARY_DIR}/ExternalData/Other)
set(Store1 ${CMAKE_BINARY_DIR}/ExternalData/Objects)
set(ExternalData_OBJECT_STORES ${Store0} ${Store1})
unset(ExternalData_URL_TEMPLATES) # All objects already in stores!
ExternalData_Add_Test(Data4
  NAME Data4Check
  COMMAND ${CMAKE_COMMAND}
    -D Data=DATA{Data.dat}
    -D Other=DATA{Other.dat}
    -D Store0=${Store0}
    -D Store1=${Store1}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/Data4Check.cmake
  )
ExternalData_Add_Target(Data4)
add_dependencies(Data4 Data3)
