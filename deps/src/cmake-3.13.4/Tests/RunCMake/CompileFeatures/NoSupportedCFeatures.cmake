
enable_language(C)

add_library(no_features empty.c)
target_compile_features(no_features PRIVATE c_static_assert)
