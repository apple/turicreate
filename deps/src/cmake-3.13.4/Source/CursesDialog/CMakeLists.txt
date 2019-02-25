# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

set( CURSES_SRCS
              CursesDialog/cmCursesOptionsWidget.cxx
              CursesDialog/cmCursesBoolWidget.cxx
              CursesDialog/cmCursesCacheEntryComposite.cxx
              CursesDialog/cmCursesDummyWidget.cxx
              CursesDialog/cmCursesFilePathWidget.cxx
              CursesDialog/cmCursesForm.cxx
              CursesDialog/cmCursesLabelWidget.cxx
              CursesDialog/cmCursesLongMessageForm.cxx
              CursesDialog/cmCursesMainForm.cxx
              CursesDialog/cmCursesPathWidget.cxx
              CursesDialog/cmCursesStringWidget.cxx
              CursesDialog/cmCursesWidget.cxx
              CursesDialog/ccmake.cxx
   )

include_directories(${CURSES_INCLUDE_PATH})


add_executable(ccmake ${CURSES_SRCS} )
target_link_libraries(ccmake CMakeLib)
if(CMAKE_USE_SYSTEM_FORM)
  target_link_libraries(ccmake
    ${CURSES_FORM_LIBRARY}
    ${CURSES_LIBRARY}
  )
  if(CURSES_EXTRA_LIBRARY)
    target_link_libraries(ccmake ${CURSES_EXTRA_LIBRARY})
  endif()
else()
  target_link_libraries(ccmake cmForm)
endif()

CMake_OPTIONAL_COMPONENT(ccmake)
install(TARGETS ccmake DESTINATION ${CMAKE_BIN_DIR} ${COMPONENT})
