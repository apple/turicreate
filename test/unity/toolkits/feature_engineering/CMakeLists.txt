project(unity_test_toolkits)

make_boost_test(feature_engineering_mock_test_model.cxx
  REQUIRES unity_shared)
make_boost_test(dict_flattening.cxx
  REQUIRES unity_shared)
make_boost_test(one_hot_encoder_test.cxx
  REQUIRES unity_shared)
make_boost_test(count_thresholder_test.cxx
  REQUIRES unity_shared)
make_boost_test(mean_imputer_test.cxx
  REQUIRES unity_shared)
make_boost_test(feature_binner_test.cxx
  REQUIRES unity_shared)
make_boost_test(tfidf.cxx
  REQUIRES unity_shared)
make_boost_test (tokenizer_test.cxx
  REQUIRES unity_shared)
make_boost_test(word_counter.cxx
  REQUIRES unity_shared)
make_boost_test(ngram_counter.cxx
  REQUIRES unity_shared)
make_boost_test(random_projection_test.cxx
  REQUIRES unity_shared)
make_boost_test(word_trimmer.cxx
  REQUIRES unity_shared)
make_boost_test(content_interpretation.cxx
  REQUIRES unity_shared)
