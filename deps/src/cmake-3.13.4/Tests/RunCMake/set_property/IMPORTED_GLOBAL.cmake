macro(print_property TARGET PROP)
  get_property(val TARGET ${TARGET} PROPERTY ${PROP})
  message(STATUS "${TARGET}: Target ${PROP} is '${val}'")
endmacro()

# Changing property on IMPORTED target created with `GLOBAL` option.
add_library(ImportedGlobalTarget SHARED IMPORTED GLOBAL)
print_property(ImportedGlobalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedGlobalTarget PROPERTY IMPORTED_GLOBAL FALSE)
print_property(ImportedGlobalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedGlobalTarget PROPERTY IMPORTED_GLOBAL TRUE)
print_property(ImportedGlobalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedGlobalTarget PROPERTY IMPORTED_GLOBAL TRUE)
print_property(ImportedGlobalTarget IMPORTED_GLOBAL)
# Appending property is never allowed!
set_property(TARGET ImportedGlobalTarget APPEND PROPERTY IMPORTED_GLOBAL TRUE)
print_property(ImportedGlobalTarget IMPORTED_GLOBAL)

# Changing property on IMPORTED target created without `GLOBAL` option.
add_library(ImportedLocalTarget SHARED IMPORTED)
print_property(ImportedLocalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedLocalTarget PROPERTY IMPORTED_GLOBAL TRUE)
print_property(ImportedLocalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedLocalTarget PROPERTY IMPORTED_GLOBAL TRUE)
print_property(ImportedLocalTarget IMPORTED_GLOBAL)
set_property(TARGET ImportedLocalTarget PROPERTY IMPORTED_GLOBAL FALSE)
print_property(ImportedLocalTarget IMPORTED_GLOBAL)

# Setting property on non-IMPORTED target is never allowed!
add_library(NonImportedTarget SHARED test.cpp)
print_property(NonImportedTarget IMPORTED_GLOBAL)
set_property(TARGET NonImportedTarget PROPERTY IMPORTED_GLOBAL TRUE)
print_property(NonImportedTarget IMPORTED_GLOBAL)

# Local IMPORTED targets can only be promoted from same directory!
add_library(ImportedLocalTarget2 SHARED IMPORTED)
print_property(ImportedLocalTarget2 IMPORTED_GLOBAL)
add_subdirectory(IMPORTED_GLOBAL)
# Note: The value should not have changed. However, it does change because the
# check for the same directory comes after it was changed! (At least, that is
# not really bad because the generation will fail due to this error.)
print_property(ImportedLocalTarget2 IMPORTED_GLOBAL)

# Global IMPORTED targets from subdir are always visible
# no matter how they became global.
print_property(ImportedSubdirTarget1 IMPORTED_GLOBAL)
print_property(ImportedSubdirTarget2 IMPORTED_GLOBAL)

# Changing property on IMPORTED target from subdir is never possible.
set_property(TARGET ImportedSubdirTarget1 PROPERTY IMPORTED_GLOBAL FALSE)
print_property(ImportedSubdirTarget1 IMPORTED_GLOBAL)
set_property(TARGET ImportedSubdirTarget2 PROPERTY IMPORTED_GLOBAL FALSE)
print_property(ImportedSubdirTarget2 IMPORTED_GLOBAL)
