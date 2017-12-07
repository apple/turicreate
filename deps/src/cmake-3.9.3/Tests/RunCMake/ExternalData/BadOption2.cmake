include(ExternalData)
ExternalData_Add_Test(Data
  NAME Test
  COMMAND ${CMAKE_COMMAND} -E echo DATA{Data.txt,Bad:Option}
  )
