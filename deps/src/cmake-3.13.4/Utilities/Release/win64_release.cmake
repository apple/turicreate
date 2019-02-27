set(CMAKE_RELEASE_DIRECTORY "c:/msys64/home/dashboard/CMakeReleaseDirectory64")
set(CONFIGURE_WITH_CMAKE TRUE)
set(CMAKE_CONFIGURE_PATH "c:/Program\\ Files/CMake/bin/cmake.exe")
set(PROCESSORS 16)
set(HOST win64)
set(RUN_LAUNCHER ~/rel/run)
set(CPACK_BINARY_GENERATORS "WIX ZIP")
set(CPACK_SOURCE_GENERATORS "")
set(MAKE_PROGRAM "ninja")
set(MAKE "${MAKE_PROGRAM} -j16")
set(qt_prefix "c:/Qt/5.6.3/msvc2017-64-xp-mt")
set(qt_win_libs
  ${qt_prefix}/plugins/platforms/qwindows.lib
  ${qt_prefix}/lib/Qt5PlatformSupport.lib
  ${qt_prefix}/lib/qtfreetype.lib
  imm32.lib
  )
set(INITIAL_CACHE "CMAKE_BUILD_TYPE:STRING=Release
CMAKE_DOC_DIR:STRING=doc/cmake
CMAKE_USE_OPENSSL:BOOL=OFF
CMAKE_SKIP_BOOTSTRAP_TEST:STRING=TRUE
CMAKE_Fortran_COMPILER:FILEPATH=FALSE
CMAKE_GENERATOR:INTERNAL=Ninja
BUILD_QtDialog:BOOL=TRUE
CMake_GUI_DISTRIBUTE_WITH_Qt_LGPL:STRING=3
CMAKE_C_FLAGS_RELEASE:STRING=-MT -O2 -Ob2 -DNDEBUG
CMAKE_CXX_FLAGS_RELEASE:STRING=-MT -O2 -Ob2 -DNDEBUG
CMAKE_EXE_LINKER_FLAGS:STRING=-machine:x64 -subsystem:console,5.02
CMake_QT_STATIC_QWindowsIntegrationPlugin_LIBRARIES:STRING=${qt_win_libs}
CMAKE_PREFIX_PATH:STRING=${qt_prefix}
CMake_TEST_Qt4:BOOL=OFF
CMake_TEST_Qt5:BOOL=OFF
")
set(ppflags "-D_WIN32_WINNT=0x502 -DNTDDI_VERSION=0x05020000 -D_USING_V110_SDK71_")
set(CFLAGS "${ppflags}")
set(CXXFLAGS "${ppflags}")
set(ENV ". ~/rel/env64")
get_filename_component(path "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(GIT_EXTRA "git config core.autocrlf true")
if(CMAKE_CREATE_VERSION STREQUAL "nightly")
  # Some tests fail spuriously too often.
  set(EXTRA_CTEST_ARGS "-E 'ConsoleBuf|Module.ExternalData'")
endif()
include(${path}/release_cmake.cmake)
