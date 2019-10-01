
cmake_minimum_required (VERSION 2.8.12)
project(Producer)

add_library(foo SHARED foo.cpp)

install(TARGETS foo EXPORT fooTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include/foo
)
install(FILES foo.h DESTINATION include/foo)
install(EXPORT fooTargets
  FILE FooConfig.cmake
  NAMESPACE Foo::
  DESTINATION lib/cmake/Foo
)

add_library(bar SHARED bar.cpp)
install(TARGETS bar
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(FILES bar.h DESTINATION include/bar)
