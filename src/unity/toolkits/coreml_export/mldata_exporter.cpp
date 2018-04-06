/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/coreml_export/mlmodel_include.hpp> 
#include <unity/toolkits/coreml_export/mldata_exporter.hpp>

namespace turi { 

/**
 *  Creates a pipeline from an MLData metadata object that takes input of the 
 *  same form as the input from the mldata would take, then outputs it as a final 
 *  vector named __vectorized_features__ that can then be used by other algorithms.
 *  The pipeline is returned.  The output variables to the pipeline are 
 */
void setup_pipeline_from_mldata(
    CoreML::Pipeline& pipeline,
    std::shared_ptr<ml_metadata> metadata) {

  CoreML::FeatureVectorizer vect = CoreML::FeatureVectorizer("");
  for(size_t column_idx = 0; column_idx < metadata->num_columns(); ++column_idx) {

    std::string column_name = metadata->column_name(column_idx);

    switch(metadata->column_mode(column_idx)) {
      case ml_column_mode::NUMERIC:
        {
          pipeline.addInput(column_name, CoreML::FeatureType::Double());
          vect.addInput(column_name, CoreML::FeatureType::Double());
          vect.add(column_name, 1);
          break;
        }

      case ml_column_mode::NUMERIC_VECTOR:
        {
          size_t dimension = metadata->index_size(column_idx);
          pipeline.addInput(column_name, CoreML::FeatureType::Array({static_cast<int64_t>(dimension)}));
          vect.addInput(column_name, CoreML::FeatureType::Array({static_cast<int64_t>(dimension)}));
          vect.add(column_name, dimension);
          break;
        }

      case ml_column_mode::NUMERIC_ND_VECTOR:
        {
          auto shape = metadata->nd_column_shape(column_idx);

#ifndef NDEBUG
          size_t s = 1;
          for(size_t e : shape) {
            s *= e;
          }
          DASSERT_EQ(s,  metadata->index_size(column_idx));
#endif
          pipeline.addInput(column_name, CoreML::FeatureType::Array(std::vector<int64_t>(shape.begin(), shape.end())));
          vect.addInput(column_name, CoreML::FeatureType::Array(std::vector<int64_t>(shape.begin(), shape.end())));
          vect.add(column_name, metadata->index_size(column_idx));
          break;
        }


      case ml_column_mode::CATEGORICAL:
      case ml_column_mode::CATEGORICAL_SORTED:
        {
          auto indexer = metadata->indexer(column_idx);

          std::string column_name = metadata->column_name(column_idx);

          CoreML::OneHotEncoder ohe =
              CoreML::OneHotEncoder(
                  "One Hot Encoder on Column" + std::to_string(column_idx));

          if(metadata->column_type(column_idx) == flex_type_enum::STRING) {
            std::vector<std::string> values(metadata->index_size(column_idx));

            for(size_t i = 0; i < metadata->index_size(column_idx); ++i) {
              values[i] = indexer->map_index_to_value(i).to<std::string>();
            }

            ohe.setFeatureEncoding(values);
          } else if(metadata->column_type(column_idx) == flex_type_enum::INTEGER) {
            std::vector<int64_t> values(metadata->index_size(column_idx));

            for(size_t i = 0; i < metadata->index_size(column_idx); ++i) {
              values[i] = indexer->map_index_to_value(i).to<int64_t>();
            }

            ohe.setFeatureEncoding(values);
          } else {
            log_and_throw(("Column " + column_name +
                           ": Only integer or string types allowed "
                           "with categorical columns.").c_str());
          }

          ohe.setHandleUnknown(MLHandleUnknownIgnoreUnknown);
          ohe.setUseSparse(true);

          size_t dimension = metadata->index_size(column_idx);

          if(metadata->column_type(column_idx) == flex_type_enum::STRING) {
            ohe.addInput(column_name, CoreML::FeatureType::String());
            ohe.addOutput(column_name, 
                CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType));

            pipeline.addInput(column_name, CoreML::FeatureType::String());
            pipeline.add(ohe);

            vect.addInput(column_name, 
                CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType));
            vect.add(column_name, dimension);

          } else if(metadata->column_type(column_idx) == flex_type_enum::INTEGER) {
            ohe.addInput(column_name, CoreML::FeatureType::Int64());
            ohe.addOutput(column_name,
                CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType));

            pipeline.addInput(column_name, CoreML::FeatureType::Int64());
            pipeline.add(ohe);

            vect.addInput(column_name, 
                CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType));
            vect.add(column_name, dimension);

          } else {
            log_and_throw(("Column " + column_name +
                           (": Only integer or string types allowed with "
                            "categorical columns.")).c_str());
          }
          break;
        }
      case ml_column_mode::DICTIONARY:
        {
          auto indexer = metadata->indexer(column_idx);
          std::string column_name = metadata->column_name(column_idx);

          CoreML::DictVectorizer dv =
              CoreML::DictVectorizer("Dict Vectorizer on Column" + std::to_string(column_idx));

          bool string_mode = false; 

          std::set<flex_type_enum> key_types = metadata->indexer(column_idx)->extract_key_types(); 

          if(key_types.size() == 1 
             && *key_types.begin() == flex_type_enum::STRING) { 

            string_mode = true;
          
          } else if(key_types.size() == 1 
                    && *key_types.begin() == flex_type_enum::INTEGER) {

            string_mode = false;

          } else { 
            log_and_throw("Only dictionary typed columns with all string or all "
                "integer keys can be exported to coreml.");
          }

          if(string_mode) {
            std::vector<std::string> values(metadata->index_size(column_idx));

            for(size_t i = 0; i < metadata->index_size(column_idx); ++i) {
              values[i] = indexer->map_index_to_value(i).to<std::string>();
            }

            dv.setFeatureEncoding(values);
          } else {
            std::vector<int64_t> values(metadata->index_size(column_idx));

            for(size_t i = 0; i < metadata->index_size(column_idx); ++i) {
              values[i] = indexer->map_index_to_value(i).to<int64_t>();
            }

            dv.setFeatureEncoding(values);
          }

          // dv.setHandleUnknown(MLHandleUnknownIgnoreUnknown);

          size_t dimension = metadata->index_size(column_idx);

          auto string_dict = CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_stringKeyType); 
          auto int_dict = CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType); 

          // Add in the correct 
          if(string_mode) {
            dv.addInput(column_name, string_dict); 
            pipeline.addInput(column_name, string_dict); 
          } else { 
            dv.addInput(column_name, int_dict); 
            dv.addOutput(column_name, int_dict);  
          }

          dv.addOutput(column_name, int_dict);  
          pipeline.add(dv);

          vect.addInput(column_name, int_dict);  
          vect.add(column_name, dimension);
          break;
        }
      case ml_column_mode::CATEGORICAL_VECTOR:
        log_and_throw("Only string, numerical, or dictionary types "
                      "allowed in exported model.");
      case ml_column_mode::UNTRANSLATED:
        DASSERT_TRUE(false);
    }
  }

  // Set the output of the vectorizer.
  vect.addOutput("__vectorized_features__",
                 CoreML::FeatureType::Array({static_cast<int64_t>(metadata->num_dimensions())}));

  // Add the vectorizer to the pipeline.
  pipeline.add(vect);
}

}
