include(CMakeAddFortranSubdirectory)
# add the fortran subdirectory as a fortran project
# the subdir is fortran, the project is FortranHello
cmake_add_fortran_subdirectory(fortran
  PROJECT FortranHello  # project name in toplevel CMakeLists.txt
  ARCHIVE_DIR ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
  RUNTIME_DIR bin # ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  LIBRARIES hello world # target libraries created
  CMAKE_COMMAND_LINE
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  LINK_LIBRARIES  # link interface libraries
   LINK_LIBS hello world  # hello needs world to link
  NO_EXTERNAL_INSTALL
  )
