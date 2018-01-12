project(unity)

make_library(
  visualization
  SOURCES
    boxes_and_whiskers.cpp
    categorical_heatmap.cpp
    groupby.cpp
    heatmap.cpp
    histogram.cpp
    hyperloglog.cpp
    io_buffer.cpp
    item_frequency.cpp
    process_wrapper.cpp
    scatter.cpp
    show.cpp
    thread.cpp
    transformation.cpp
    vega_data.cpp
    vega_spec.cpp
  REQUIRES
    EXTERNAL_VISIBILITY
)

set(GENERATED_VEGA_JSON_HEADERS
  vega_spec/boxes_and_whiskers.json
  vega_spec/categorical.json
  vega_spec/categorical_heatmap.json
  vega_spec/heatmap.json
  vega_spec/histogram.json
  vega_spec/scatter.json
  vega_spec/summary_view.json
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vega_spec)

foreach(SOURCE_FILE ${GENERATED_VEGA_JSON_HEADERS})

  string(
    REGEX REPLACE
    ".json$"
    ".h"
    GENERATED_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}
  )

  add_custom_command(
    OUTPUT ${GENERATED_FILE}
    COMMAND xxd -i ${SOURCE_FILE} > ${GENERATED_FILE}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ${GENERATED_FILE}"
    VERBATIM
  )
  
  string(
    MAKE_C_IDENTIFIER
    ${SOURCE_FILE}
    TARGET_NAME
  )

  add_custom_target(
    Generate_${TARGET_NAME}
    DEPENDS
    ${GENERATED_FILE}
  )

  add_dependencies(visualization Generate_${TARGET_NAME})

endforeach(SOURCE_FILE)
