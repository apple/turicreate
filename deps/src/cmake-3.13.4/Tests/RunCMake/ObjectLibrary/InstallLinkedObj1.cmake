add_library(A OBJECT a.c)
add_library(UseA STATIC)
target_link_libraries(UseA PUBLIC A)

install(TARGETS UseA EXPORT exp ARCHIVE DESTINATION lib)
install(EXPORT exp DESTINATION lib/cmake/exp)
