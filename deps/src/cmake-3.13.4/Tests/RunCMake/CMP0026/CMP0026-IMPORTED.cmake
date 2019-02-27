
enable_language(CXX)

add_library(someimportedlib SHARED IMPORTED)

get_target_property(_loc someimportedlib LOCATION)
