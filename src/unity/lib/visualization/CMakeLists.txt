project(unity)

# HACK HACK HACK
# columnwise_summary.cpp should be in this file, not the parent directory,
# but including it here for some reason causes linker errors
# in release/test (C++ unit tests) - but not debug! And only on Linux.
# Not sure why.

if(APPLE)
  set(VISUALIZATION_PLATFORM_SOURCES plot_macos.mm)
endif()

add_library(visualization OBJECT
  batch_size.cpp
  boxes_and_whiskers.cpp
  categorical_heatmap.cpp
  dark_mode.cpp
  escape.cpp
  groupby.cpp
  heatmap.cpp
  histogram.cpp
  io_buffer.cpp
  item_frequency.cpp
  plot.cpp
  process_wrapper.cpp
  registration.cpp
  scatter.cpp
  show.cpp
  summary_view.cpp
  thread.cpp
  transformation.cpp
  vega_data.cpp
  vega_spec.cpp
  ${VISUALIZATION_PLATFORM_SOURCES}
)
add_dependencies(visualization boost)

set(GENERATED_VEGA_JSON_HEADERS
  vega_spec/boxes_and_whiskers.json
  vega_spec/categorical.json
  vega_spec/categorical_heatmap.json
  vega_spec/config.json
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
