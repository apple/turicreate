set(CMAKE_RELEASE_DIRECTORY "c:/msys64/home/dashboard/CMakeReleaseDirectory")
set(CONFIGURE_WITH_CMAKE TRUE)
set(CMAKE_CONFIGURE_PATH "c:/Program\\ Files/CMake/bin/cmake.exe")
set(PROCESSORS 8)
set(HOST dash3win7)
set(RUN_LAUNCHER ~/rel/run)
set(CPACK_BINARY_GENERATORS "WIX ZIP")
set(CPACK_SOURCE_GENERATORS "ZIP")
set(MAKE_PROGRAM "ninja")
set(MAKE "${MAKE_PROGRAM} -j8")
set(INITIAL_CACHE "CMAKE_BUILD_TYPE:STRING=Release
CMAKE_DOC_DIR:STRING=doc/cmake
CMAKE_USE_OPENSSL:BOOL=OFF
CMAKE_SKIP_BOOTSTRAP_TEST:STRING=TRUE
CMAKE_Fortran_COMPILER:FILEPATH=FALSE
CMAKE_GENERATOR:INTERNAL=Ninja
BUILD_QtDialog:BOOL:=TRUE
CMake_ENABLE_SERVER_MODE:BOOL=TRUE
CMake_GUI_DISTRIBUTE_WITH_Qt_LGPL:STRING=3
CMake_INSTALL_DEPENDENCIES:BOOL=ON
CMAKE_EXE_LINKER_FLAGS:STRING=-machine:x86 -subsystem:console,5.01
")
set(ppflags "-D_WIN32_WINNT=0x501 -DNTDDI_VERSION=0x05010000 -D_USING_V110_SDK71_")
set(CFLAGS "${ppflags}")
set(CXXFLAGS "${ppflags}")
set(ENV ". ~/rel/env")
get_filename_component(path "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(GIT_EXTRA "git config core.autocrlf true")
if(CMAKE_CREATE_VERSION STREQUAL "nightly")
  # Some tests fail spuriously too often.
  set(EXTRA_CTEST_ARGS "-E 'Qt5Autogen|ConsoleBuf'")
endif()
include(${path}/release_cmake.cmake)
