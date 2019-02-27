
enable_language(CXX)

add_library(testTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")

if (TEST_PROP STREQUAL INCLUDE_DIRECTORIES)
  set_property(TARGET testTarget PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$<1:foo>")
else()
  set_property(TARGET testTarget PROPERTY INTERFACE_SOURCES "$<1:empty.cpp>")
endif()

add_library(userTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")
target_link_libraries(userTarget testTarget)
