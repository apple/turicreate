# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

set( CURSES_SRCS
              CursesDialog/cmCursesOptionsWidget
              CursesDialog/cmCursesBoolWidget
              CursesDialog/cmCursesCacheEntryComposite
              CursesDialog/cmCursesDummyWidget
              CursesDialog/cmCursesFilePathWidget
              CursesDialog/cmCursesForm
              CursesDialog/cmCursesLabelWidget
              CursesDialog/cmCursesLongMessageForm
              CursesDialog/cmCursesMainForm
              CursesDialog/cmCursesPathWidget
              CursesDialog/cmCursesStringWidget
              CursesDialog/cmCursesWidget
              CursesDialog/ccmake
   )

if( NOT CMAKE_USE_SYSTEM_FORM )
  include_directories(${CMake_SOURCE_DIR}/Source/CursesDialog/form
                      ${CMake_BINARY_DIR}/Source/CursesDialog/form)
endif()
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
