set(ExternalData_SERIES_PARSE "([0-9]+)(_\\.my\\.dat)$")
set(ExternalData_SERIES_MATCH "([0-9]+)")
ExternalData_Add_Test(Data2
  NAME Data2Check
  COMMAND ${CMAKE_COMMAND}
    -D Data2=DATA{../Data2.dat}
    -D Data2b=DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data2b.dat}
    -D SeriesC=DATA{SeriesC_1_.my.dat,:}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/Data2Check.cmake
  )
ExternalData_Add_Target(Data2)
