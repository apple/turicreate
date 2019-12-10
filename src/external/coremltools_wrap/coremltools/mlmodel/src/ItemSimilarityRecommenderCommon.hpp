  //
  //  ItemSimilarityRecommenderCommon.hpp
  //  CoreML_framework
  //
  //  Created by Hoyt Koepke on 1/30/19.
  //  Copyright © 2019 Apple Inc. All rights reserved.
  //

#ifndef ItemSimilarityRecommenderCommon_hpp
#define ItemSimilarityRecommenderCommon_hpp

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <cstdint>

namespace CoreML {

  namespace Specification {
    class Model;
    class ItemSimilarityRecommender;
  }

  namespace Recommender {

    /** The struct holding the data for the recommender.
     *
     */
    struct _ItemSimilarityRecommenderData {

      _ItemSimilarityRecommenderData(){}
      _ItemSimilarityRecommenderData(const Specification::ItemSimilarityRecommender& isr);
      bool operator==(const _ItemSimilarityRecommenderData& other) const;

      std::map<uint64_t, std::vector<std::pair<uint64_t, double> > > item_interactions;
      std::map<uint64_t, double> item_shift_values;
      uint64_t num_items;
      std::string item_restriction_input_column;
      std::string num_recommendations_input_column;
      std::string item_exclusion_input_column;
      std::string item_data_input_column;

      std::string item_list_output_column;
      std::string item_score_output_column;

      std::vector<int64_t> integer_id_values;
      std::vector<std::string> string_id_values;

    };

    /** If the model is a tree, then it will have tree ensemble
     *  tendencies.
     */
    std::shared_ptr<_ItemSimilarityRecommenderData>
    constructAndValidateItemSimilarityRecommenderFromSpec(const Specification::Model& isr);

  }
}

#endif /* ItemSimilarityRecommenderCommon_hpp */
