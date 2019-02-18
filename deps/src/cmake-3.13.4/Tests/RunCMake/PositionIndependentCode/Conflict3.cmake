
add_library(picoff UNKNOWN IMPORTED)

add_library(picon UNKNOWN IMPORTED)
set_property(TARGET picon PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

add_executable(conflict "main.cpp")
target_link_libraries(conflict picon)
set_property(TARGET conflict APPEND PROPERTY
  LINK_LIBRARIES
    $<$<NOT:$<BOOL:$<TARGET_PROPERTY:POSITION_INDEPENDENT_CODE>>>:picoff>
)
