install(FILES main.c DESTINATION src-all)
install(FILES main.c DESTINATION src-uns EXCLUDE_FROM_ALL)
install(FILES main.c DESTINATION src-exc EXCLUDE_FROM_ALL COMPONENT exc)
