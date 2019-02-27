
add_library(A OBJECT IMPORTED)

# We don't actually build this example so just configure dummy
# object files to test.  They do not have to exist.
set_property(TARGET A APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(A PROPERTIES
  IMPORTED_OBJECTS_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/does_not_exist.o"
  IMPORTED_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/does_not_exist.o"
  )

add_library(B $<TARGET_OBJECTS:A> b.c)
