enable_language(CXX)

add_library(main INTERFACE)
target_compile_features(main PRIVATE cxx_delegating_constructors)
