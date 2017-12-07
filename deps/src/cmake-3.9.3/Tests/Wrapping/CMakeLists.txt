#
# Wrapping
#
cmake_minimum_required (VERSION 2.6)
project (Wrapping)

# Disable cleaning of custom command outputs to preserve the hacks
# used to generate the files using CONFIGURE_FILE.
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

#
# Lib and exe path
#
set (LIBRARY_OUTPUT_PATH
  ${Wrapping_BINARY_DIR}/bin/ CACHE INTERNAL
  "Single output directory for building all libraries.")

set (EXECUTABLE_OUTPUT_PATH
  ${Wrapping_BINARY_DIR}/bin/ CACHE INTERNAL
  "Single output directory for building all executables.")

#
# Where will executable tests be written ?
#
if (EXECUTABLE_OUTPUT_PATH)
  set (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})
else ()
  set (CXX_TEST_PATH .)
endif ()

#
# Add exe
#
add_executable (wrapping wrapping.cxx)

add_executable (Wrap Wrap.c)
if(WIN32)
  set(EXE_EXT ".exe")
endif()
set(WRAP ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/Wrap${EXE_EXT})

#
# Qt Wrappers
#

set (QT_WRAP_CPP "On")
set (QT_MOC_EXE "echo")
include( FindQt3 )

if (QT_FOUND AND QT_WRAP_UI)
  message("found Qt 3 test it...")
  include_directories( ${QT_INCLUDE_DIR} )
  include_directories( ${CMAKE_CURRENT_BINARY_DIR} )


  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/foo.ui.in
    ${CMAKE_CURRENT_BINARY_DIR}/foo.ui)

  set (QT_WRAP_UI "On")
  set (QT_UIC_EXE "${QT_UIC_EXECUTABLE}")


  set (QTUI_SRCS
    qtwrapping.ui
    ${CMAKE_CURRENT_BINARY_DIR}/foo.ui
    )
  qt_wrap_ui (myqtlib QTUI_H_SRCS QTUI_S_SRCS ${QTUI_SRCS})
  qt_wrap_cpp (myqtlib QT_MOC_SRCS ${SRCS} vtkTestMoc.h)

  message("Qt files are ${QTUI_S_SRCS}")
  message("Qt other files are ${QTUI_H_SRCS}")
  add_definitions(${QT_DEFINITIONS})
  add_library(myqtlib ${QTUI_S_SRCS} ${QT_MOC_SRCS})
  add_executable (qtwrapping qtwrappingmain.cxx)
  target_link_libraries(qtwrapping myqtlib)

  target_link_libraries( qtwrapping ${QT_LIBRARIES} )
else ()
  add_executable (qtwrapping qtnoqtmain.cxx)
endif ()

#
# FLTK Wrappers
#
# Since FLTK_FLUID_EXE is supposed to create a .cxx/.h from a .fl/.fld,
# create an empty one so that the dependencies can be met.
#
add_executable(fakefluid fakefluid.cxx)
set (FLTK_WRAP_UI "On")
set (FLTK_FLUID_EXECUTABLE fakefluid)
fltk_wrap_ui (wraplibFLTK fltk1.fl)
add_library(wraplibFLTK ${wraplibFLTK_FLTK_UI_SRCS})
add_dependencies(wraplibFLTK fakefluid)
add_dependencies(fakefluid Wrap)
fltk_wrap_ui (wrapFLTK fltk2.fl)
add_executable(wrapFLTK wrapFLTK.cxx ${wrapFLTK_FLTK_UI_SRCS})
target_link_libraries(wrapFLTK wraplibFLTK)
add_dependencies(wrapFLTK fakefluid)

#
# Mangled Mesa
#
configure_file(
  ${Wrapping_SOURCE_DIR}/dummy
  ${Wrapping_BINARY_DIR}/gl.h
  COPYONLY)
use_mangled_mesa (${Wrapping_BINARY_DIR} ${Wrapping_BINARY_DIR}/mangled_mesa)

