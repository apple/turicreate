enable_language(C)

add_executable(myexe main.c)

install_files(/src FILES empty.c)
install_files(/src .c obj1)
install_files(/src "^obj2.c$")

install_targets(/bin myexe)

install_programs(/scripts1 FILES script script.bat)
install_programs(/scripts2 script script.bat)
install_programs(/scripts3 "^script(\.bat)?$")
