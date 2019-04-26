foreach(parameter OUTPUT_NAME COMPRESSION_FLAGS DECOMPRESSION_FLAGS)
  if(NOT DEFINED ${parameter})
    message(FATAL_ERROR "missing required parameter ${parameter}")
  endif()
endforeach()

function(run_tar WORKING_DIRECTORY)
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar ${ARGN}
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
    RESULT_VARIABLE result
  )

  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "tar failed with arguments [${ARGN}] result [${result}]")
  endif()
endfunction()

set(COMPRESS_DIR compress_dir)
set(FULL_COMPRESS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${COMPRESS_DIR})

set(DECOMPRESS_DIR decompress_dir)
set(FULL_DECOMPRESS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DECOMPRESS_DIR})

set(FULL_OUTPUT_NAME ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME})

set(CHECK_FILES
  "f1.txt"
  "d1/f1.txt"
  "d 2/f1.txt"
  "d + 3/f1.txt"
  "d_4/f1.txt"
  "d-4/f1.txt"
  "My Special Directory/f1.txt"
)

foreach(file ${CHECK_FILES})
  configure_file(${CMAKE_CURRENT_LIST_FILE} ${FULL_COMPRESS_DIR}/${file} COPYONLY)
endforeach()

if(UNIX)
  execute_process(COMMAND ln -sf f1.txt ${FULL_COMPRESS_DIR}/d1/f2.txt)
  list(APPEND CHECK_FILES "d1/f2.txt")
endif()

file(REMOVE ${FULL_OUTPUT_NAME})
file(REMOVE_RECURSE ${FULL_DECOMPRESS_DIR})
file(MAKE_DIRECTORY ${FULL_DECOMPRESS_DIR})

run_tar(${CMAKE_CURRENT_BINARY_DIR} ${COMPRESSION_FLAGS} ${FULL_OUTPUT_NAME} ${COMPRESSION_OPTIONS} ${COMPRESS_DIR})
run_tar(${FULL_DECOMPRESS_DIR} ${DECOMPRESSION_FLAGS} ${FULL_OUTPUT_NAME} ${DECOMPRESSION_OPTIONS})

foreach(file ${CHECK_FILES})
  set(input ${FULL_COMPRESS_DIR}/${file})
  set(output ${FULL_DECOMPRESS_DIR}/${COMPRESS_DIR}/${file})

  if(NOT EXISTS ${input})
     message(SEND_ERROR "Cannot find input file ${output}")
  endif()

  if(NOT EXISTS ${output})
     message(SEND_ERROR "Cannot find output file ${output}")
  endif()

  file(MD5 ${input} input_md5)
  file(MD5 ${output} output_md5)

  if(NOT input_md5 STREQUAL output_md5)
    message(SEND_ERROR "Files \"${input}\" and \"${output}\" are different")
  endif()
endforeach()

function(check_magic EXPECTED)
  file(READ ${FULL_OUTPUT_NAME} ACTUAL
    ${ARGN}
  )

  if(NOT ACTUAL STREQUAL EXPECTED)
    message(FATAL_ERROR
      "Actual [${ACTUAL}] does not match expected [${EXPECTED}]")
  endif()
endfunction()
