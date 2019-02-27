enable_language(C)

add_executable(main empty.c)
target_compile_features(main
  PRIVATE
    c_not_a_feature
)
