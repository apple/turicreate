enable_language(C)

add_library(namelink-lib empty.c)

install(TARGETS namelink-lib
  DESTINATION lib
  COMPONENT lib
  NAMELINK_COMPONENT dev
)
