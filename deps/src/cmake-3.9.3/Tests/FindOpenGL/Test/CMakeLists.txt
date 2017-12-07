cmake_minimum_required(VERSION 3.7)
project(TestFindOpenGL C)
include(CTest)

find_package(OpenGL REQUIRED)

add_executable(test_tgt main.c)
target_link_libraries(test_tgt OpenGL::GLU)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${OPENGL_INGLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${OPENGL_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
