set(PROCESSORS 4)
set(BOOTSTRAP_ARGS "--docdir=doc/cmake")
set(HOST linux64)
set(MAKE_PROGRAM "make")
set(CPACK_BINARY_GENERATORS "STGZ TGZ")
set(CC /opt/gcc-6.1.0/bin/gcc)
set(CXX /opt/gcc-6.1.0/bin/g++)
set(CFLAGS   "")
set(CXXFLAGS "")
set(qt_prefix "/home/kitware/qt-5.7.0")
set(qt_xcb_libs
  ${qt_prefix}/plugins/platforms/libqxcb.a
  ${qt_prefix}/lib/libQt5XcbQpa.a
  ${qt_prefix}/lib/libQt5PlatformSupport.a
  ${qt_prefix}/lib/libxcb-static.a
  -lX11-xcb
  -lX11
  -lxcb
  -lfontconfig
  -lfreetype
  )
set(INITIAL_CACHE "
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_C_STANDARD:STRING=11
CMAKE_CXX_STANDARD:STRING=14
CMAKE_C_FLAGS:STRING=-D_POSIX_C_SOURCE=199506L -D_POSIX_SOURCE=1 -D_SVID_SOURCE=1 -D_BSD_SOURCE=1
CMAKE_EXE_LINKER_FLAGS:STRING=-static-libstdc++ -static-libgcc
CURSES_LIBRARY:FILEPATH=/home/kitware/ncurses-5.9/lib/libncurses.a
CURSES_INCLUDE_PATH:PATH=/home/kitware/ncurses-5.9/include
FORM_LIBRARY:FILEPATH=/home/kitware/ncurses-5.9/lib/libform.a
CMAKE_USE_OPENSSL:BOOL=ON
OPENSSL_CRYPTO_LIBRARY:FILEPATH=/home/kitware/openssl-1.0.2j/lib/libcrypto.a
OPENSSL_INCLUDE_DIR:PATH=/home/kitware/openssl-1.0.2j/include
OPENSSL_SSL_LIBRARY:FILEPATH=/home/kitware/openssl-1.0.2j/lib/libssl.a
PYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3
CPACK_SYSTEM_NAME:STRING=Linux-x86_64
BUILD_QtDialog:BOOL:=TRUE
CMAKE_SKIP_BOOTSTRAP_TEST:STRING=TRUE
CMake_ENABLE_SERVER_MODE:BOOL=TRUE
CMake_GUI_DISTRIBUTE_WITH_Qt_LGPL:STRING=3
CMake_INSTALL_DEPENDENCIES:BOOL=ON
CMAKE_PREFIX_PATH:STRING=${qt_prefix}
CMake_QT_STATIC_QXcbIntegrationPlugin_LIBRARIES:STRING=${qt_xcb_libs}
")

# Exclude Qt5 tests because our Qt5 is static.
set(EXTRA_CTEST_ARGS "-E Qt5")

get_filename_component(path "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${path}/release_cmake.cmake)
