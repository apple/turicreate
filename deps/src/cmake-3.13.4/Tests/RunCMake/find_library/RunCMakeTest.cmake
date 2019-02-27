include(RunCMake)

run_cmake(Created)
if(CMAKE_HOST_UNIX)
  run_cmake(LibArchLink)
endif()
if(WIN32 OR CYGWIN)
  run_cmake(PrefixInPATH)
endif()
