enable_language(CXX)

add_executable(main empty.cpp)
target_compile_features(main INVALID cxx_delegating_constructors)
