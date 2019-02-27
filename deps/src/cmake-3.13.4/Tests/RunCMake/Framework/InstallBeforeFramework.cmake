enable_language(C)

add_library(foo SHARED foo.c)
install(TARGETS foo LIBRARY DESTINATION lib)
set_property(TARGET foo PROPERTY FRAMEWORK TRUE)
