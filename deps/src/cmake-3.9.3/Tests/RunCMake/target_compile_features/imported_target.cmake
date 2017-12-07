enable_language(CXX)

add_library(main INTERFACE IMPORTED)
target_compile_features(main INTERFACE cxx_delegating_constructors)
