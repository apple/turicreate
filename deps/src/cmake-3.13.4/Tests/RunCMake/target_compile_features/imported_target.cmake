enable_language(CXX)

add_library(lib1-interface INTERFACE IMPORTED)
target_compile_features(lib1-interface INTERFACE cxx_delegating_constructors)

add_library(lib2-interface INTERFACE IMPORTED)
target_compile_features(lib2-interface PUBLIC cxx_delegating_constructors)

add_library(lib-shared SHARED IMPORTED)
target_compile_features(lib-shared INTERFACE cxx_delegating_constructors)
