include(GenerateExportHeader)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(libshared SHARED libshared.cpp)

generate_export_header(libshared)

export(TARGETS libshared FILE Targets.cmake)
