enable_language(C)
add_executable(myexe main.c)
add_subdirectory(TARGETS-InstallFromSubDir)
install(TARGETS myexe subexe DESTINATION bin)
