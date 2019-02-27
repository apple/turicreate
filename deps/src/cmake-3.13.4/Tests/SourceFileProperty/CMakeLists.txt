cmake_minimum_required(VERSION 3.0)
project(SourceFileProperty C)

set(sources)

if (EXISTS icasetest.c)
  # If a file exists by this name, use it.
  set_source_files_properties(icasetest.c
    PROPERTIES
      COMPILE_FLAGS -DNEEDED_TO_WORK)
else ()
  # Work on case-sensitive file systems as well.
  set_source_files_properties(main.c
    PROPERTIES
      COMPILE_FLAGS -DNO_NEED_TO_CALL)
endif ()
list(APPEND sources ICaseTest.c)

add_executable(SourceFileProperty main.c ${sources})
