include("${INFO_FILE}")

foreach(source ${SOURCES})
  file(WRITE "CMakeFiles/${TARGET}.dir/${source}.gcno"
    "${SOURCE_DIR}/${source}"
  )
endforeach()
