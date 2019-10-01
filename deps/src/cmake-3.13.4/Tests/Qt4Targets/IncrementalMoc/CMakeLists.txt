
cmake_minimum_required(VERSION 2.8.12)
project(IncrementalMoc)

find_package(Qt4 REQUIRED)

if (ADD_TARGET_DEF)
  set(target_args TARGET testlib)
endif()

if (ADD_DIR_DEF)
  add_definitions(-DNEW_DEF)
endif()

qt4_generate_moc(foo.h moc_foo.cpp ${target_args})

add_library(testlib foo.cpp moc_foo.cpp)
target_link_libraries(testlib Qt4::QtCore)
if (ADD_TARGET_DEF)
  target_compile_definitions(testlib PRIVATE NEW_DEF)
endif()
