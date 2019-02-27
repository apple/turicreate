include(RunCMake)

run_cmake(NoSources)
run_cmake(OnlyObjectSources)
if(NOT RunCMake_GENERATOR STREQUAL "Xcode" OR NOT "$ENV{CMAKE_OSX_ARCHITECTURES}" MATCHES "[;$]")
  run_cmake(NoSourcesButLinkObjects)
endif()
