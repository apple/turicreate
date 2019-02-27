add_library(bar SHARED ../empty.c)
install(TARGETS bar DESTINATION lib EXPORT fooExport)
