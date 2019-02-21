cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)
project(TestFindOpenGL C)
include(CTest)

find_package(OpenGL REQUIRED)

# import target for GLU
add_executable(test_tgt main.c)
target_link_libraries(test_tgt OpenGL::GLU)
add_test(NAME test_tgt COMMAND test_tgt)

# OPENGL_LIBRARIES should be whatever libraries are needed to link.
add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${OPENGL_INGLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${OPENGL_LIBRARIES})
add_test(NAME test_var COMMAND test_var)

# VND support adds an ::OpenGL import target.  This can be used for OpenGL-only
# code (code that does not manipulate contexts, like our 'main.c').  Without
# VND, ::GL can be used for both context and non-context OpenGL code.
if(OpenGL_TEST_VND)
  add_executable(test_comp_none main.c)
  target_link_libraries(test_comp_none PRIVATE OpenGL::OpenGL)
  add_test(NAME test_comp_none COMMAND test_comp_none)
else()
  add_executable(test_comp_none main.c)
  target_link_libraries(test_comp_none PRIVATE OpenGL::GL)
  add_test(NAME test_comp_none COMMAND test_comp_none)
endif()

# GLX
if(OpenGL_TEST_VND)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX)
  add_executable(test_comp_glx main.c)
  target_link_libraries(test_comp_glx PRIVATE OpenGL::OpenGL OpenGL::GLX)
  add_test(NAME test_comp_glx COMMAND test_comp_glx)
else()
  # non-VND systems won't have it, but an optional search for GLX should still
  # be okay.
  find_package(OpenGL COMPONENTS GLX)
  add_executable(test_comp_glx_novnd main.c)
  target_link_libraries(test_comp_glx_novnd PRIVATE OpenGL::GL)
  add_test(NAME test_comp_glx_novnd COMMAND test_comp_glx_novnd)
endif()

# EGL is only available on Linux+GLVND at present.
if(OpenGL_TEST_VND)
  find_package(OpenGL COMPONENTS OpenGL EGL)
  if(OpenGL_EGL_FOUND)
    add_executable(test_comp_egl main.c)
    target_link_libraries(test_comp_egl PRIVATE OpenGL::OpenGL OpenGL::EGL)
    add_test(NAME test_comp_egl COMMAND test_comp_egl)
    # EGL-only code should not link to GLX.
    execute_process(COMMAND ldd test_comp_egl
                    OUTPUT_VARIABLE LDD_OUT
                    ERROR_VARIABLE LDD_ERR)
    if("${LDD_OUT}" MATCHES "GLX")
      message(FATAL_ERROR "EGL-only code links to GLX!")
    endif()
  endif()

  # all three COMPONENTS together.
  find_package(OpenGL COMPONENTS OpenGL EGL GLX)
  if(OpenGL_EGL_FOUND AND OpenGL_GLX_FOUND)
    add_executable(test_comp_both main.c)
    target_link_libraries(test_comp_both PRIVATE OpenGL::OpenGL OpenGL::EGL
                          OpenGL::GLX)
    add_test(NAME test_comp_both COMMAND test_comp_both)
  endif()
endif()
