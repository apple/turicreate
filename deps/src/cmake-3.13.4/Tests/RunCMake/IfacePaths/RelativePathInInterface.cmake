
enable_language(CXX)

add_library(testTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")
if (TEST_PROP STREQUAL INCLUDE_DIRECTORIES)
  set_property(TARGET testTarget PROPERTY INTERFACE_INCLUDE_DIRECTORIES "foo")
else()
  set_property(TARGET testTarget PROPERTY INTERFACE_SOURCES "empty.cpp")
endif()
install(TARGETS testTarget EXPORT testTargets
  DESTINATION lib
)

install(EXPORT testTargets DESTINATION lib/cmake)
