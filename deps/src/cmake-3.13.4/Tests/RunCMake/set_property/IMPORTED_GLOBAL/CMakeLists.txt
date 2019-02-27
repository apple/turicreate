add_library(ImportedSubdirTarget1 SHARED IMPORTED GLOBAL)
add_library(ImportedSubdirTarget2 SHARED IMPORTED)

# Extend visibility of ImportedSubdirTarget2 to global scope.
set_property(TARGET ImportedSubdirTarget2 PROPERTY IMPORTED_GLOBAL TRUE)

# Only targets from the same directory can be promoted.
set_property(TARGET ImportedLocalTarget2 PROPERTY IMPORTED_GLOBAL TRUE)
