
enable_language(C)

cmake_policy(SET CMP0026 OLD)

add_subdirectory(clear-cached-information-dir)

# Critical: this needs to happen in root CMakeLists.txt and not inside
# the subdir.
get_target_property(mypath Hello LOCATION)
# Now we create the file later, so you can see, ultimately no error should
# happen e.g. during generate phase:
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/clear-cached-information-dir/main.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/clear-cached-information-dir/main.c PROPERTIES GENERATED TRUE)
