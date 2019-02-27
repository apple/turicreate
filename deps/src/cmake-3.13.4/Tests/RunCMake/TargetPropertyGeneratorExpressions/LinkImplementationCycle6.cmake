
add_library(empty1 SHARED empty.cpp)
add_library(empty2 SHARED empty.cpp)

# The INTERFACE_INCLUDE_DIRECTORIES do not depend on the link interface.
# On its own, this is fine. It is only when used by empty3 that an error
# is appropriately issued.
target_link_libraries(empty1
  INTERFACE
    $<$<STREQUAL:$<TARGET_PROPERTY:INTERFACE_INCLUDE_DIRECTORIES>,/foo/bar>:empty2>
)

add_library(empty3 SHARED empty.cpp)
target_link_libraries(empty3 empty1)
