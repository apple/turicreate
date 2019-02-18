
cmake_minimum_required(VERSION 2.8)

project(ExportLanguagesTest)

include(${CMAKE_CURRENT_BINARY_DIR}/../BExport.cmake)
get_property(configs TARGET ExpExportLanguagesB PROPERTY IMPORTED_CONFIGURATIONS)
foreach(c ${configs})
  get_property(langs TARGET ExpExportLanguagesB PROPERTY IMPORTED_LINK_INTERFACE_LANGUAGES_${c})
  list(FIND langs CXX pos)
  if(${pos} LESS 0)
    message(FATAL_ERROR "Target export does not list object library languages.")
  endif()
endforeach()
