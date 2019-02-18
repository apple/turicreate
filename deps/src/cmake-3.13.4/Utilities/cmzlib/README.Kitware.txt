This directory contains a subset of the zlib library (1.2.3) and
some custom changes.

We only include enough of the distribution to provide the functionalities
required.

We would like to thank the zlib team for distributing this library.
http://www.zlib.net

Added Files
-----------

CMakeLists.txt
  -Support building with CMake.

zlib.rc
  -For MS Windows only: provide a version resource in a dll build so that
   when you look at the dll file in Windows explorer, it will show you the
   version in the "Version" tab of the file's properties view.

zlib.def
  -For MS Windows only: used to explicitly list the exports from dll builds.

cm_zlib_mangle.h
  -Mangles symbols exported from the zlib library for use by CMake.

zlibDllConfig.h.in
  -Configures the correct value of the ZLIB_DLL define based on the
   BUILD_SHARED_LIBS CMake option.

Changed Files
-------------
You can search the code for "KITWARE_ZLIB_CHANGE" to find modifications
vs the original zlib code

zconf.h
  -Include cm_zlib_mangle.h (at the top)
  -Include cmzlib/zlibDllConfig.h (at the top)
  -Changed an #if 0 to #ifdef HAVE_UNISTD_H (near middle)
  -Suppress selected compiler warnings (at the bottom)
