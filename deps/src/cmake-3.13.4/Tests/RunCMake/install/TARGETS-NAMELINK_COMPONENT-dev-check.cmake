if(WIN32 OR CYGWIN OR NO_NAMELINK)
  set(_check_files)
else()
  set(_check_files
    [[lib]]
    [[lib/libnamelink-only\.(so|dylib)]]
    [[lib/libnamelink-sep\.(so|dylib)]]
    [[lib/libnamelink-uns-dev\.(so|dylib)]]
  )
endif()
check_installed("^${_check_files}$")
