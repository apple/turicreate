foreach(I RANGE 0 ${CMAKE_ARGC})
  if("${CMAKE_ARGV${I}}" MATCHES ".*\\.gcda")
    set(gcda_file "${CMAKE_ARGV${I}}")
  endif()
endforeach()

get_filename_component(gcda_name ${gcda_file} NAME)
string(REPLACE ".gcda" ".gcov" gcov_name "${gcda_name}")

file(STRINGS "${gcda_file}" source_file LIMIT_COUNT 1 ENCODING UTF-8)

file(WRITE "${CMAKE_SOURCE_DIR}/${gcov_name}"
  "        -:    0:Source:${source_file}"
)
