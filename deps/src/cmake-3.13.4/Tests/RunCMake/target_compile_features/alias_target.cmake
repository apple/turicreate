enable_language(CXX)

add_executable(main empty.cpp)
add_executable(Alias::Main ALIAS main)
target_compile_features(Alias::Main PRIVATE cxx_delegating_constructors)
