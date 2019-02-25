set(CMAKE_SIZEOF_VOID_P 8)
set(CMAKE_LIBRARY_ARCHITECTURE "arch")
include(GNUInstallDirs)
set(dirs
  BINDIR
  DATADIR
  DATAROOTDIR
  DOCDIR
  INCLUDEDIR
  INFODIR
  LIBDIR
  LIBEXECDIR
  LOCALEDIR
  LOCALSTATEDIR
  RUNSTATEDIR
  MANDIR
  SBINDIR
  SHAREDSTATEDIR
  SYSCONFDIR
  )
foreach(dir ${dirs})
  message("CMAKE_INSTALL_${dir}='${CMAKE_INSTALL_${dir}}'")
endforeach()
foreach(dir ${dirs})
  message("CMAKE_INSTALL_FULL_${dir}='${CMAKE_INSTALL_FULL_${dir}}'")
endforeach()
