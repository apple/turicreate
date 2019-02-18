cmake_minimum_required(VERSION 2.8.10.20130115)
project(ExternalDataTest NONE)

include(CTest)

include(ExternalData)

if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "^/")
  set(slash /)
endif()
set(ExternalData_URL_TEMPLATES
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/%(algo)/%(hash)"
  "file://${slash}${CMAKE_CURRENT_SOURCE_DIR}/Alt/%(algo:MyAlgoMap1)/%(hash)"
  "ExternalDataCustomScript://MyScript1/%(algo)/%(hash)"
  )
set(ExternalData_URL_ALGO_MD5_MyAlgoMap1 MyAlgoMap1-md5)
set(ExternalData_CUSTOM_SCRIPT_MyScript1 "${CMAKE_CURRENT_SOURCE_DIR}/MyScript1.cmake")
set(ExternalData_BINARY_ROOT "${CMAKE_CURRENT_BINARY_DIR}/ExternalData")
file(REMOVE_RECURSE ${ExternalData_BINARY_ROOT}) # clean test

if(MAKE_SUPPORTS_SPACES)
  set(Data1CheckSpaces -D "DataSpace=DATA{Data Space.dat}")
endif()

ExternalData_Add_Test(Data1
  NAME Data1Check
  COMMAND ${CMAKE_COMMAND}
    -D Data=DATA{Data.dat}
    ${Data1CheckSpaces}
    -D DataScript=DATA{DataScript.dat}
    -D DataAlgoMapA=DATA{DataAlgoMapA.dat}
    -D DataAlgoMapB=DATA{DataAlgoMapB.dat}
    -D DataMissing=DATA{DataMissing.dat}
    -D DataMissingWithAssociated=DATA{DataMissing.dat,Data.dat}
    -D SeriesA=DATA{SeriesA.dat,:}
    -D SeriesB=DATA{SeriesB.dat,:}
    -D SeriesC=DATA{SeriesC.dat,:}
    -D SeriesD=DATA{SeriesD.dat,:}
    -D SeriesAn=DATA{SeriesAn1.dat,:}
    -D SeriesBn=DATA{SeriesBn_1.dat,:}
    -D SeriesCn=DATA{SeriesCn.1.dat,:}
    -D SeriesDn=DATA{SeriesDn-1.dat,:}
    -D SeriesMixed=DATA{SeriesMixed.1.dat,:}
    -D Paired=DATA{PairedA.dat,PairedB.dat}
    -D Meta=DATA{MetaTop.dat,REGEX:Meta[ABC].dat}
    -D Directory=DATA{Directory/,A.dat,REGEX:[BC].dat}
    -D DirRecurse=DATA{DirRecurse/,RECURSE:,A.dat,REGEX:[BC].dat}
    -D MultipleAlgorithmNoSHA1=DATA{MultipleAlgorithmNoSHA1.dat}
    -D MultipleAlgorithmNoMD5=DATA{MultipleAlgorithmNoMD5.dat}
    -D "Semicolons=DATA{Data.dat}\\;DATA{Data.dat}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/Data1Check.cmake
  )
ExternalData_Add_Target(Data1)

add_subdirectory(Data2)
add_subdirectory(Data3)
add_subdirectory(Data4)
add_subdirectory(Data5)
add_subdirectory(DataNoSymlinks)
