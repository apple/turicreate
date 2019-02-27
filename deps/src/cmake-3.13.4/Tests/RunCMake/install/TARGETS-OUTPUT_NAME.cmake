enable_language(C)

add_executable(test1 main.c)
set_property(TARGET test1 PROPERTY OUTPUT_NAME test1out)
set_property(TARGET test1 PROPERTY RELEASE_OUTPUT_NAME test1rel)

add_executable(test2 main.c)
set_property(TARGET test2 PROPERTY OUTPUT_NAME test2out)
set_property(TARGET test2 PROPERTY DEBUG_OUTPUT_NAME test2deb)

add_executable(test3 main.c)
set_property(TARGET test3 PROPERTY RUNTIME_OUTPUT_NAME test3exc)

add_library(test4 SHARED obj1.c)
set_property(TARGET test4 PROPERTY LIBRARY_OUTPUT_NAME test4lib)

add_library(test5 STATIC obj1.c)
set_property(TARGET test5 PROPERTY ARCHIVE_OUTPUT_NAME test5ar)

install(TARGETS
  test1
  test2
  test3
  test4
  test5
  DESTINATION bin
  )
