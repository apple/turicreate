enable_language(C)
add_executable(main main.c)
set_property(SOURCE main.c PROPERTY COMPILE_FLAGS "$<$<CONFIG:Debug>:-DMYDEBUG>")
