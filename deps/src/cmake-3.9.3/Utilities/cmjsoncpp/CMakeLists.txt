project(JsonCpp CXX)

# Disable warnings to avoid changing 3rd party code.
if(CMAKE_CXX_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PathScale")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -woffall")
endif()

set(JSONCPP_SOURCES
  src/lib_json/json_batchallocator.h
  src/lib_json/json_reader.cpp
  src/lib_json/json_tool.h
  src/lib_json/json_value.cpp
  src/lib_json/json_valueiterator.inl
  src/lib_json/json_writer.cpp
  )

include_directories(
  ${JsonCpp_SOURCE_DIR}/include
  ${KWSYS_HEADER_ROOT}
  )

add_library(cmjsoncpp ${JSONCPP_SOURCES})
target_link_libraries(cmjsoncpp ${CMake_KWIML_LIBRARIES})
set_property(TARGET cmjsoncpp PROPERTY CXX_INCLUDE_WHAT_YOU_USE "")
