
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GenerateExportHeader)
add_library(excludedFromAll SHARED EXCLUDE_FROM_ALL excludedFromAll.cpp)
generate_export_header(excludedFromAll)
