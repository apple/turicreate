enable_language(C)
add_executable(main main.c)
set_property(SOURCE main.c PROPERTY INCLUDE_DIRECTORIES "$<$<CONFIG:Debug>:MYDEBUG>")
