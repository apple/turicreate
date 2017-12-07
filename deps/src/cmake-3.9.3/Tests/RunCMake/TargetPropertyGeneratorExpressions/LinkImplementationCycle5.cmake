
add_library(empty1 INTERFACE IMPORTED)
add_library(empty2 INTERFACE IMPORTED)

set_property(TARGET empty1 PROPERTY INTERFACE_LINK_LIBRARIES
  $<$<STREQUAL:$<TARGET_PROPERTY:INTERFACE_INCLUDE_DIRECTORIES>,/foo/bar>:empty2>
)

add_library(empty3 empty.cpp)
target_link_libraries(empty3 empty1)
