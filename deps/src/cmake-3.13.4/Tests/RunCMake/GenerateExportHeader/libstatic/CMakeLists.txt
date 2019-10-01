set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GenerateExportHeader)

# Show that the export header has no effect on a static library.

add_library(libstatic STATIC libstatic.cpp)

generate_export_header(libstatic)

export(TARGETS libstatic FILE Targets.cmake)
