
add_library(renamed_on_export SHARED renamed.cxx)
generate_export_header(renamed_on_export)
target_include_directories(renamed_on_export
  PUBLIC "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/renamed>"
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>"
)
set_property(TARGET renamed_on_export
  PROPERTY
    EXPORT_NAME renamed
)

install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/renamed.h"
  "${CMAKE_CURRENT_BINARY_DIR}/renamed_on_export_export.h"
    DESTINATION include/renamed
)

install(TARGETS renamed_on_export
        EXPORT exp DESTINATION lib )
