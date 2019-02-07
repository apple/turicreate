project(unity_toolkits)

add_library(unity_feature_engineering OBJECT
  class_registrations.cpp
  dict_transform_utils.cpp
  transform_to_flat_dictionary.cpp
  one_hot_encoder.cpp
  feature_binner.cpp
  sample_transformer.cpp
  dimension_reduction.cpp
  quadratic_features.cpp
  topk_indexer.cpp
  statistics_tracker.cpp
  count_thresholder.cpp
  mean_imputer.cpp
  bm25.cpp
  tfidf.cpp
  tokenizer.cpp
  categorical_imputer.cpp
  word_counter.cpp
  ngram_counter.cpp
  count_featurizer.cpp
  word_trimmer.cpp
  content_interpretation.cpp
)
add_dependencies(unity_feature_engineering boost)