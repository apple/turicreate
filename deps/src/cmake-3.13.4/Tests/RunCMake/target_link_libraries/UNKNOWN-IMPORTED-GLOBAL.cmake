enable_language(C)
add_library(UnknownImportedGlobal UNKNOWN IMPORTED GLOBAL)
add_library(mylib empty.c)
target_link_libraries(mylib UnknownImportedGlobal)
