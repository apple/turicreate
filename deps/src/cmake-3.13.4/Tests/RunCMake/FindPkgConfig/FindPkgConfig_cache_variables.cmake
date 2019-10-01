cmake_minimum_required(VERSION 3.3)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES QUIET ncurses)

if (NCURSES_FOUND)
  foreach (variable IN ITEMS PREFIX INCLUDEDIR LIBDIR)
    get_property(value
      CACHE     "NCURSES_${variable}"
      PROPERTY  VALUE)
    if (NOT value STREQUAL NCURSES_${variable})
      message(FATAL_ERROR "Failed to set cache entry for NCURSES_${variable}:\nexpected -->${value}<--\nreceived -->${NCURSES_${variable}}<--")
    endif ()
  endforeach ()
else ()
  message(STATUS "skipping test; ncurses not found")
endif ()
