enable_language(CXX)
add_library(testTarget empty.cpp)

if (TEST_PROP STREQUAL INCLUDE_DIRECTORIES)
  set_property(TARGET testTarget PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/dir")
else()
  set_property(TARGET testTarget PROPERTY INTERFACE_SOURCES "${CMAKE_INSTALL_PREFIX}/empty.cpp")
endif()

install(TARGETS testTarget EXPORT testTargets
  DESTINATION lib
)

install(EXPORT testTargets DESTINATION lib/cmake)
