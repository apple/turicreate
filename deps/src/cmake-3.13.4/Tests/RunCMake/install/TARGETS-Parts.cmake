enable_language(C)
add_library(mylib STATIC obj1.c)
set_property(TARGET mylib PROPERTY PUBLIC_HEADER obj1.h)
install(TARGETS mylib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
  )
