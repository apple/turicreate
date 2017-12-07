project(network)

if(WIN32)
  SET(PLATFORM_DEPENDENCIES iphlpapi ws2_32)
endif()

make_library(network
  SOURCES
   net_util.cpp
  REQUIRES
   logger ${PLATFORM_DEPENDENCIES}
)
