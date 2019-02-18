if(CMAKE_VERSION MATCHES "\\.(20[0-9][0-9])[0-9][0-9][0-9][0-9](-|$)")
  set(version_year "${CMAKE_MATCH_1}")
  set(copyright_line_regex "^Copyright 2000-(20[0-9][0-9]) Kitware")
  file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../Copyright.txt" copyright_line
    LIMIT_COUNT 1 REGEX "${copyright_line_regex}")
  if(copyright_line MATCHES "${copyright_line_regex}")
    set(copyright_year "${CMAKE_MATCH_1}")
    if(copyright_year LESS version_year)
      message(FATAL_ERROR "Copyright.txt contains\n"
        " ${copyright_line}\n"
        "but the current version year is ${version_year}.")
    else()
      message(STATUS "PASSED: Copyright.txt contains\n"
        " ${copyright_line}\n"
        "and the current version year is ${version_year}.")
    endif()
  else()
    message(FATAL_ERROR "Copyright.txt has no Copyright line of expected format!")
  endif()
else()
  message(STATUS "SKIPPED: CMAKE_VERSION does not know the year: ${CMAKE_VERSION}")
endif()
