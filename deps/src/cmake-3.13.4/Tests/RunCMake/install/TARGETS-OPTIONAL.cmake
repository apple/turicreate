enable_language(C)
add_executable(myexe main.c)
add_executable(notall EXCLUDE_FROM_ALL main.c)
install(TARGETS myexe notall DESTINATION bin OPTIONAL)
