include(FetchContent)

file(WRITE tmpFile.txt "Generated contents, not important")

FetchContent_Populate(
  t1
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_BINARY_DIR}/tmpFile.txt
                   <SOURCE_DIR>/done1.txt
)
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/t1-src/done1.txt)
  message(FATAL_ERROR "Default SOURCE_DIR doesn't contain done1.txt")
endif()

FetchContent_Populate(
  t2
  SOURCE_DIR       ${CMAKE_CURRENT_BINARY_DIR}/mysrc
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_BINARY_DIR}/tmpFile.txt
                   <SOURCE_DIR>/done2.txt
)
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/mysrc/done2.txt)
  message(FATAL_ERROR "Specified SOURCE_DIR doesn't contain done2.txt")
endif()

FetchContent_Populate(
  t3
  SOURCE_DIR       myrelsrc
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_CURRENT_BINARY_DIR}/tmpFile.txt
                   <SOURCE_DIR>/done3.txt
)
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/myrelsrc/done3.txt)
  message(FATAL_ERROR "Relative SOURCE_DIR doesn't contain done3.txt")
endif()
