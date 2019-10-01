
enable_language(CXX)

set(CMAKE_DEBUG_TARGET_PROPERTIES INCLUDE_DIRECTORIES)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/DebugIncludes.cpp" "enum { dummy };\n")

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/one"
  "${CMAKE_CURRENT_SOURCE_DIR}/two"
)

set_property(DIRECTORY APPEND PROPERTY INCLUDE_DIRECTORIES
                          "${CMAKE_CURRENT_SOURCE_DIR}/three")

add_library(lll "${CMAKE_CURRENT_BINARY_DIR}/DebugIncludes.cpp")

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/two"
  "${CMAKE_CURRENT_SOURCE_DIR}/three"
  "${CMAKE_CURRENT_SOURCE_DIR}/four"
)

add_library(foo "${CMAKE_CURRENT_BINARY_DIR}/DebugIncludes.cpp")
target_include_directories(foo
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/seven"
    "${CMAKE_CURRENT_SOURCE_DIR}/eight"
)
target_link_libraries(lll foo)

macro(some_macro)
  set_property(TARGET lll APPEND PROPERTY
      INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/one"
                          "${CMAKE_CURRENT_SOURCE_DIR}/three"
                          "${CMAKE_CURRENT_SOURCE_DIR}/four"
                          "${CMAKE_CURRENT_SOURCE_DIR}/five"
                          "${CMAKE_CURRENT_SOURCE_DIR}/six"
                          "${CMAKE_CURRENT_SOURCE_DIR}/seven"
  )
endmacro()

function(some_function)
  some_macro()
endfunction()

some_function()

add_library(bar "${CMAKE_CURRENT_BINARY_DIR}/DebugIncludes.cpp")
target_include_directories(bar
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/nine"
    "${CMAKE_CURRENT_SOURCE_DIR}/ten"
)
set_property(TARGET lll APPEND PROPERTY LINK_LIBRARIES bar)
