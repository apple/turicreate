# a simple test case
project (Simple)

add_executable (Simple simple.cxx)

add_library (simpleLib STATIC
  simpleLib.cxx
  simpleCLib.c
  simpleWe.cpp
  )

target_link_libraries (Simple simpleLib)

# make sure optimized libs are not used by debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(Simple optimized c:/not/here.lib )
endif()
