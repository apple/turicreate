include(CTest)
include(ExternalData)
ExternalData_Add_Test(Data
  NAME Test
  COMMAND ${CMAKE_COMMAND} -E echo DATA{Directory4/,:}
  )
