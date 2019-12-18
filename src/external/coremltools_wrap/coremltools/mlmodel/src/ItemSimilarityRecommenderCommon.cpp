  //
  //  ItemSimilarityRecommenderCommon.cpp
  //  CoreML_framework
  //
  //  Created by Hoyt Koepke on 1/30/19.
  //  Copyright © 2019 Apple Inc. All rights reserved.
  //

#include "ItemSimilarityRecommenderCommon.hpp"
#include "ValidatorUtils-inl.hpp"
#include "Validators.hpp"
#include "Format.hpp"

namespace CoreML { namespace Recommender {

  _ItemSimilarityRecommenderData::_ItemSimilarityRecommenderData(const Specification::ItemSimilarityRecommender& isr) {

    // Validate that we have item_ids in the correct 0, 1, ..., n-1 sequence.
    std::set<uint64_t> index_hits;

    int n_similarities = isr.itemitemsimilarities_size();

    for(int i = 0; i < n_similarities; ++i) {
      const auto& item_sim_info = isr.itemitemsimilarities(i);
      uint64_t item_id = item_sim_info.itemid();
      index_hits.insert(item_id);

      auto& interaction_list_dest = item_interactions[item_id];
      int n_interactions = item_sim_info.similaritemlist_size();

      for(int j = 0; j < n_interactions; ++j) {
        const auto& interaction = item_sim_info.similaritemlist(j);
        uint64_t inter_id = interaction.itemid();
        double score = interaction.similarityscore();

        interaction_list_dest.push_back({inter_id, score});

        index_hits.insert(inter_id);
      }

      // Sort to ensure equality between equivalent models.
      std::sort(interaction_list_dest.begin(), interaction_list_dest.end());
      double score_shift = item_sim_info.itemscoreadjustment();
      if(score_shift != 0) {
        item_shift_values[item_id] = score_shift;
      }
    }

    if(index_hits.size() <= *index_hits.rbegin()) {

      // Make sure that all the model actually numbers things correctly.
      throw std::invalid_argument("Item IDs in the recommender model must be numbered 0, 1, ..., num_items - 1.");
    }

    num_items = index_hits.size();

    // Check out the item similarity
    if(isr.has_itemint64ids() && isr.itemint64ids().vector_size() != 0) {
      if(isr.has_itemstringids() && isr.itemstringids().vector_size() != 0) {
        throw std::invalid_argument("Only integer item ids or string item ids can be specified in the same model.");
      }

      if(size_t(isr.itemint64ids().vector_size()) != num_items) {
        throw std::invalid_argument("Number of integer item ids specified ("
                                    + std::to_string(isr.itemint64ids().vector_size())
                                    + ") does not equal the number of items given ("
                                    + std::to_string(num_items) + ")");
      }

      integer_id_values.resize((size_t)num_items);
      for(size_t i = 0; i < num_items; ++i) {
        integer_id_values[i] = isr.itemint64ids().vector(int(i));
      }
    } else if(isr.has_itemstringids() && isr.itemstringids().vector_size() != 0) {
        if(size_t(isr.itemstringids().vector_size()) != num_items) {
          throw std::invalid_argument("Number of string item ids specified ("
                                      + std::to_string(isr.itemstringids().vector_size())
                                      + ") does not equal the number of items given ("
                                      + std::to_string(num_items) + ")");
        }

        string_id_values.resize((size_t)num_items);
        for(size_t i = 0; i < num_items; ++i) {
          string_id_values[i] = isr.itemstringids().vector(int(i));
        }
    }

    // Check out the specific parameters
    item_data_input_column = isr.iteminputfeaturename();
    num_recommendations_input_column = isr.numrecommendationsinputfeaturename();
    item_exclusion_input_column = isr.itemexclusioninputfeaturename();
    item_restriction_input_column = isr.itemrestrictioninputfeaturename();

    // Get the item output columns.
    item_list_output_column = isr.recommendeditemlistoutputfeaturename();
    item_score_output_column = isr.recommendeditemscoreoutputfeaturename();
  }

  bool _ItemSimilarityRecommenderData::operator==(const _ItemSimilarityRecommenderData& other) const {
    return (num_items == other.num_items
            && item_interactions == other.item_interactions
            && item_shift_values == other.item_shift_values
            && item_restriction_input_column == other.item_restriction_input_column
            && num_recommendations_input_column == other.num_recommendations_input_column
            && item_exclusion_input_column == other.item_exclusion_input_column
            && item_data_input_column == other.item_data_input_column
            && item_list_output_column == other.item_list_output_column
            && item_score_output_column == other.item_score_output_column
            && string_id_values == other.string_id_values
            && integer_id_values == other.integer_id_values);

  }

  /** If the model is a tree, then it will have tree ensemble
   *  tendencies.
   */
  std::shared_ptr<_ItemSimilarityRecommenderData>
  constructAndValidateItemSimilarityRecommenderFromSpec(const Specification::Model& spec){
    const auto& interface = spec.description();

      // Validate its a MLModel type.
    auto result = validateModelDescription(interface, spec.specificationversion());

    if(!result.good()) {
      throw std::invalid_argument(result.message());
    }

    /** Preliminary. -- get the right tree parameters out to get all the nodes.
     */
    if(!spec.has_itemsimilarityrecommender()) {
      throw std::invalid_argument(result.message());
    }

    auto isr = spec.itemsimilarityrecommender();

    auto ret = std::make_shared<_ItemSimilarityRecommenderData>(isr);

    // Perform validation on the constructed data set

    // Validate the input column
    {
      if(ret->item_data_input_column.empty()) {
        if(interface.input_size() == 1) {
          ret->item_data_input_column = interface.input(0).name();
        } else {
          throw std::invalid_argument("Name of column for item input data not specified.");
        }
      }

      std::vector<Specification::FeatureType::TypeCase> allowed_item_input_types =
      { Specification::FeatureType::kDictionaryType,
        Specification::FeatureType::kSequenceType,
        Specification::FeatureType::kMultiArrayType};

      result = validateDescriptionsContainFeatureWithNameAndType(interface.input(),
                                                                 ret->item_data_input_column,
                                                                 allowed_item_input_types);
      if(!result.good()) {
        throw std::invalid_argument(result.message());
      }
    }

    // Now validate the num recommendations input
    {

      if(!ret->num_recommendations_input_column.empty()) {
        result = validateDescriptionsContainFeatureWithNameAndType(interface.input(),
                                                                   ret->num_recommendations_input_column,
                                                                   {Specification::FeatureType::kInt64Type});
        if(!result.good()) {
          throw std::invalid_argument(result.message());
        }
      }
    }

    // Now test the more advanced item inclusion and exclusion columns.
    {
      std::vector<Specification::FeatureType::TypeCase> allowed_item_input_types =
      { Specification::FeatureType::kSequenceType,
        Specification::FeatureType::kMultiArrayType};

      for(const auto& n : {ret->item_exclusion_input_column, ret->item_restriction_input_column}) {
        if(!n.empty()) {
          result = validateDescriptionsContainFeatureWithNameAndType(interface.input(),
                                                                     n,
                                                                     allowed_item_input_types);

          if(!result.good()) {
            throw std::invalid_argument(result.message());
          }
        }
      }
    }

    // Now validate the num recommendations output
    bool output_column_specified = false;
    if(!ret->item_score_output_column.empty()) {
      result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                 ret->item_score_output_column,
                                                                 {Specification::FeatureType::kDictionaryType});
      if(!result.good()) {
        throw std::invalid_argument(result.message());
      }
      output_column_specified = true;
    }

    // Now validate the num recommendations output
    if(!ret->item_list_output_column.empty()) {
      result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                 ret->item_list_output_column,
                                                                 {Specification::FeatureType::kSequenceType});
      if(!result.good()) {
        throw std::invalid_argument(result.message());
      }
      output_column_specified = true;
    }

    if(!output_column_specified) {
      throw std::invalid_argument("No output columns specified.");
    }

    return ret;
  }


}}
