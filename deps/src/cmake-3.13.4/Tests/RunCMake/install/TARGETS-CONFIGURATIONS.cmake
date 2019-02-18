install(FILES main.c DESTINATION src CONFIGURATIONS Debug RENAME main-d.c)
install(FILES main.c DESTINATION src CONFIGURATIONS Release RENAME main-r.c)
