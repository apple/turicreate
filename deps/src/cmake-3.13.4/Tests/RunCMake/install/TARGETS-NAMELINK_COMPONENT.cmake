enable_language(C)

macro(add_versioned_library NAME)
  add_library(${NAME} SHARED obj1.c)
  set_target_properties(${NAME} PROPERTIES
    VERSION 1.0
    SOVERSION 1
  )
endmacro()

add_versioned_library(namelink-sep)
add_versioned_library(namelink-same)
add_versioned_library(namelink-uns)
add_versioned_library(namelink-uns-dev)
add_versioned_library(namelink-only)
add_versioned_library(namelink-skip)
add_library(namelink-none SHARED obj1.c)

install(TARGETS namelink-sep namelink-none
  RUNTIME
    DESTINATION lib
    COMPONENT lib
  LIBRARY
    DESTINATION lib
    COMPONENT lib
    NAMELINK_COMPONENT dev
)
install(TARGETS namelink-same
  RUNTIME
    DESTINATION lib
    COMPONENT lib
  LIBRARY
    DESTINATION lib
    COMPONENT lib
)
install(TARGETS namelink-uns
  RUNTIME
    DESTINATION lib
  LIBRARY
    DESTINATION lib
)
install(TARGETS namelink-uns-dev
  RUNTIME
    DESTINATION lib
  LIBRARY
    DESTINATION lib
    NAMELINK_COMPONENT dev
)
install(TARGETS namelink-only
  RUNTIME
    DESTINATION lib
    COMPONENT lib
  LIBRARY
    DESTINATION lib
    COMPONENT lib
    NAMELINK_COMPONENT dev
    NAMELINK_ONLY
)
install(TARGETS namelink-skip
  RUNTIME
    DESTINATION lib
    COMPONENT lib
  LIBRARY
    DESTINATION lib
    COMPONENT lib
    NAMELINK_COMPONENT dev
    NAMELINK_SKIP
)
