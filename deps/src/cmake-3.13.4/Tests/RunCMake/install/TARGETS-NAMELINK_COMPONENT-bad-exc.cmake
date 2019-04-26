enable_language(C)

add_executable(namelink-exc main.c)

install(TARGETS namelink-exc
  RUNTIME
    DESTINATION bin
    COMPONENT exc
    NAMELINK_COMPONENT dev
)
