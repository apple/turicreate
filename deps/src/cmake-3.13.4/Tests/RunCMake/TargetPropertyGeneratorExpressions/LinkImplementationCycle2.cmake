
add_library(empty1 empty.cpp)
add_library(empty2 empty.cpp)

target_link_libraries(empty1
  LINK_PUBLIC
    $<$<STREQUAL:$<TARGET_PROPERTY:INTERFACE_INCLUDE_DIRECTORIES>,/foo/bar>:empty2>
)
