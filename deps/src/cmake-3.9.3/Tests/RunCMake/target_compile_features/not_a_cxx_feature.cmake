enable_language(CXX)

add_executable(main empty.cpp)
target_compile_features(main
  PRIVATE
    cxx_not_a_feature
)
