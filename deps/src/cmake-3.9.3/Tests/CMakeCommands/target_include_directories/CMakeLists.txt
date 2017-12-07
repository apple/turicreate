cmake_minimum_required(VERSION 2.8)

project(target_include_directories)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/privateinclude")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/privateinclude.h" "#define PRIVATEINCLUDE_DEFINE\n")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/publicinclude")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/publicinclude/publicinclude.h" "#define PUBLICINCLUDE_DEFINE\n")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/interfaceinclude")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/interfaceinclude/interfaceinclude.h" "#define INTERFACEINCLUDE_DEFINE\n")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/poison")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/poison/common.h" "#error Should not be included\n")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cure")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cure/common.h" "#define CURE_DEFINE\n")

add_executable(target_include_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
target_include_directories(target_include_directories
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/privateinclude"
  PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/publicinclude"
  INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/interfaceinclude"
)

target_include_directories(target_include_directories
  PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/poison"
)
target_include_directories(target_include_directories
  BEFORE PUBLIC "$<$<STREQUAL:$<TARGET_PROPERTY:target_include_directories,TYPE>,EXECUTABLE>:${CMAKE_CURRENT_BINARY_DIR}/cure>"
)

# Has no effect because the target type is not SHARED_LIBRARY:
target_include_directories(target_include_directories
  BEFORE PUBLIC "$<$<STREQUAL:$<TARGET_PROPERTY:target_include_directories,TYPE>,SHARED_LIBRARY>:${CMAKE_CURRENT_BINARY_DIR}/poison>"
)

add_executable(consumer
  "${CMAKE_CURRENT_SOURCE_DIR}/consumer.cpp"
)

if (CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
  target_sources(consumer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/consumer.c"
  )
  target_include_directories(consumer
    PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/cxx_only>
      $<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/c_only>
  )
  target_compile_definitions(consumer
    PRIVATE -DTEST_LANG_DEFINES
  )
endif()

target_include_directories(consumer
  PRIVATE
    $<TARGET_PROPERTY:target_include_directories,INTERFACE_INCLUDE_DIRECTORIES>
    relative_dir
    relative_dir/$<TARGET_PROPERTY:NAME>
)

# Test no items
target_include_directories(consumer
  PRIVATE
)
target_include_directories(consumer
  BEFORE PRIVATE
)
target_include_directories(consumer
  SYSTEM BEFORE PRIVATE
)
target_include_directories(consumer
  SYSTEM PRIVATE
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(imp UNKNOWN IMPORTED)
get_target_property(_res imp INCLUDE_DIRECTORIES)
if (_res)
  message(SEND_ERROR "include_directories populated the INCLUDE_DIRECTORIES target property")
endif()
