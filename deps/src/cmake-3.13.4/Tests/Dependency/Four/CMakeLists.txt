include_directories(${Dependency_BINARY_DIR}/Two)
add_library( Four FourSrc.c )
target_link_libraries( Four One Two NoDepA )

# TwoCustom must build before Four.
add_dependencies(Four TwoCustom)
