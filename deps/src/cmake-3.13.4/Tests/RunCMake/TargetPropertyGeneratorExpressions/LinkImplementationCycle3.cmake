
add_library(empty1 empty.cpp)
add_library(empty2 empty.cpp)

# This is OK, because evaluating the INCLUDE_DIRECTORIES is not affected by
# the content of the INTERFACE_LINK_LIBRARIES.
target_link_libraries(empty1
  INTERFACE
    $<$<STREQUAL:$<TARGET_PROPERTY:INCLUDE_DIRECTORIES>,/foo/bar>:empty2>
)
