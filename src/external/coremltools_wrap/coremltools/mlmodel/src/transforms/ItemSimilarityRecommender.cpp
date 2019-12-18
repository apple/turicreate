//
//  ItemSimilarityRecommender.cpp
//  CoreML_framework
//
//  Created by Hoyt Koepke on 1/29/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "ItemSimilarityRecommender.hpp"
#include "../ItemSimilarityRecommenderCommon.hpp"
#include "../Format.hpp"
#include <memory>

namespace CoreML {
  ItemSimilarityRecommender::ItemSimilarityRecommender(const std::string& description)
  : Model(description)
  , m_isr_data(new Recommender::_ItemSimilarityRecommenderData)
  , m_isr(m_spec->mutable_itemsimilarityrecommender())
  {
  }
  
  
  
  /**  Set the similarity of the given item to the another item.
   *
   *   If the user has item id1 in their observed interactions
   *   with a rating of obs_value, then for each item linked to
   *   the reference items of reference_item_id=id1, the value
   *
   *      link_value * (obs_value - item_shift_value)
   *
   *   is added to the score of link_item_id.
   *
   *   If symmetric is true, then it is equivalent to calling t
   *   this method twice and swapping the reference_item_id and
   *   the linked_item_id.
   */
  void ItemSimilarityRecommender::addItemItemInteraction(size_t reference_item_id, size_t linked_item_id, double link_value, bool symmetric)
  {
    m_isr_data->item_interactions[reference_item_id].push_back({linked_item_id, link_value});
    
    if(symmetric) {
      m_isr_data->item_interactions[linked_item_id].push_back({reference_item_id, link_value});
    }
  }
  
  /** Sets the adjustment value of this item that is applied to the user's rating of the item.  This may be used to adjust for biases in the item values relative to the user's score.
   */
  void ItemSimilarityRecommender::setItemShiftValue(size_t item_id, double value) {
    m_isr_data->item_shift_values[item_id] = value;
  }
  
  /** Sets the name of the input data feature.  If include_scores is True, than the
   *  expected input is a dictionary of item to score.  If it is false, it's a
   *  MLMultiArray of item ids.
   */
  void ItemSimilarityRecommender::setItemDataInputFeatureName(const std::string& name, bool include_scores) {
    use_dictionary_input = include_scores;
    m_isr_data->item_data_input_column = name;
  }
  
  /** Sets the name of the column that dictates how many recommended
   *  items are returned by the model.
   */
  void ItemSimilarityRecommender::setNumRecommendationsInputFeatureName(const std::string& name) {
    m_isr_data->num_recommendations_input_column = name;
  }
  
  /** Sets the name of the column that allows the user to restrict recommended items to
   *  a specific set.
   */
  void ItemSimilarityRecommender::setItemRestrictionInputFeatureName(const std::string& name) {
    // Just to be consistent
    m_isr_data->item_restriction_input_column = name;
  }
  
  /** Sets the name of the column that allows the user to exclude items from recommendation.
   *  If given, this overrides the default behavior, which is to exclude all the items
   *  in the input item list.  Specifying this parameter and then passing in an empty
   *  array of item ids causes the recommendations to come from all available items.
   */
  void ItemSimilarityRecommender::setItemExclusionInputFeatureName(const std::string& name) {
    m_isr_data->item_exclusion_input_column = name;
  }
  
  
  /** Set the column name for the Item Id output.  This is a sequence of at most K items,
   *  ordered from highest scored to lowest scored.
   */
  void ItemSimilarityRecommender::setRecommendedItemIdOutputName(const std::string& name) {
    m_isr_data->item_list_output_column = name;
  }
  
  /** Set the column name for the Item score output.  This is a dictionary of Item ID
   *  to score.
   */
  void ItemSimilarityRecommender::setRecommendedItemScoreOutputName(const std::string& name) {
    
    m_isr_data->item_score_output_column = name;
  }
  
  /**  Sets a mapping of the integer values of the items from their index values above
   *   to an integer ID.  Input items are assumed to be this ID, and outputs are given as such
   *   an ID.  If neither setItemIntegerList or setItemStringList is called, the item IDs
   *   are assumed to match the indices above.
   */
  void ItemSimilarityRecommender::setItemIntegerList(const std::vector<int64_t>& integer_items) {
    m_isr_data->integer_id_values = integer_items;
    m_isr_data->string_id_values.clear();
  }
  
  /**  Sets a mapping of the integer indices of the items from their index values above
   *   to a string ID.  Input items are assumed to be this ID, and outputs are given as such
   *   an ID.  If neither setItemIntegerList or setItemStringList is called, the item IDs
   *   are assumed to match the indices above.
   */
  void ItemSimilarityRecommender::setItemStringList(const std::vector<std::string>& string_items) {
    m_isr_data->string_id_values = string_items;
    m_isr_data->integer_id_values.clear();
  }
  
  /** Saves and validates the current model.
   */
  void ItemSimilarityRecommender::finish() {
    
    // Are we using string items?
    bool string_items = (m_isr_data->string_id_values.size() != 0);
    
    // Deal with the input items column.
    {
      Specification::FeatureDescription* fd = new Specification::FeatureDescription;
      std::string name = m_isr_data->item_data_input_column;
      if(name.empty()) {
        name = "items";
      }
      
      fd->set_name(m_isr_data->item_data_input_column);
      fd->set_shortdescription("The list of items used to generate the recommendations. ");
      
      Specification::FeatureType* type = new Specification::FeatureType;
      
      if(use_dictionary_input) {
        if(string_items) {
          type->mutable_dictionarytype()->mutable_stringkeytype();
        } else {
          type->mutable_dictionarytype()->mutable_int64keytype();
        }
      } else {
        if(string_items) {
          type->mutable_sequencetype()->mutable_stringtype();
        } else {
          type->mutable_sequencetype()->mutable_int64type();
        }
      }
      
      fd->set_allocated_type(type);
      m_spec->mutable_description()->mutable_input()->AddAllocated(fd);
      m_spec->mutable_itemsimilarityrecommender()->set_iteminputfeaturename(
                                                                            m_isr_data->item_data_input_column);
    }
    
    // Deal with the number of recommendations as input
    {
      std::string name = m_isr_data->num_recommendations_input_column;
      if(name.empty()) {
        name = "k";
      }

      if(!name.empty()) {
        Specification::FeatureDescription* fd = new Specification::FeatureDescription;
        fd->set_name(name);
        fd->set_shortdescription("The number of items to return on a recommendation.");
        
        Specification::FeatureType* type = new Specification::FeatureType;
        
        type->mutable_int64type();
        
        fd->set_allocated_type(type);
        m_spec->mutable_description()->mutable_input()->AddAllocated(fd);
        m_spec->mutable_itemsimilarityrecommender()->set_numrecommendationsinputfeaturename(name);
      }
    }
    
    // Item restriction list
    {
      const std::string& name = m_isr_data->item_restriction_input_column;
      if(!name.empty()) {
        Specification::FeatureDescription* fd = new Specification::FeatureDescription;
        fd->set_name(name);
        fd->set_shortdescription("A sequence of items from which to generate recommendations.");
        
        Specification::FeatureType* type = new Specification::FeatureType;
        
        if(string_items) {
          type->mutable_sequencetype()->mutable_stringtype();
        } else {
          type->mutable_sequencetype()->mutable_int64type();
        }
        
        type->set_isoptional(true);

        fd->set_allocated_type(type);

        m_spec->mutable_description()->mutable_input()->AddAllocated(fd);
        m_spec->mutable_itemsimilarityrecommender()->set_itemrestrictioninputfeaturename(name);
      }
    }
    
    // Item exclusion list
    {
      const std::string& name = m_isr_data->item_exclusion_input_column;
      if(!name.empty()) {
        Specification::FeatureDescription* fd = new Specification::FeatureDescription;
        fd->set_name(name);
        fd->set_shortdescription("A sequence of items to exclude from recommendations.  Defaults to the input item list if not given.");
        
        Specification::FeatureType* type = new Specification::FeatureType;
        
        if(string_items) {
          type->mutable_sequencetype()->mutable_stringtype();
        } else {
          type->mutable_sequencetype()->mutable_int64type();
        }
        type->set_isoptional(true);

        fd->set_allocated_type(type);
        m_spec->mutable_description()->mutable_input()->AddAllocated(fd);
        m_spec->mutable_itemsimilarityrecommender()->set_itemexclusioninputfeaturename(name);
      }
    }
    
    // Sequence of recommended items
    {
      std::string name = m_isr_data->item_list_output_column;
      
      if(name.empty()) {
        name = "recommendations";
      }
      
      auto* output = m_spec->mutable_description()->mutable_output();
      
      Specification::FeatureDescription* fd = new Specification::FeatureDescription;
      fd->set_name(name);
      fd->set_shortdescription("The recommended items in order from most relevant to least relevant.");
      
      Specification::FeatureType* outputType = new Specification::FeatureType;
      if(string_items) {
        outputType->mutable_sequencetype()->mutable_stringtype();
      } else {
        outputType->mutable_sequencetype()->mutable_int64type();
      }
      m_spec->mutable_itemsimilarityrecommender()->set_recommendeditemlistoutputfeaturename(name);
      
      fd->set_allocated_type(outputType);
      output->AddAllocated(fd);
    }
    
    {
      const std::string& name = m_isr_data->item_score_output_column;
      
      if(!name.empty()) {
        auto* output = m_spec->mutable_description()->mutable_output();
        
        Specification::FeatureDescription* fd = new Specification::FeatureDescription;
        
        fd->set_name(name);
        fd->set_shortdescription("The scores for the recommended items, given as a "
                                 "dictionary of items and the corresponding scores.");
        
        Specification::FeatureType* outputType = new Specification::FeatureType;
        if(string_items) {
          outputType->mutable_dictionarytype()->mutable_stringkeytype();
        } else {
          outputType->mutable_dictionarytype()->mutable_int64keytype();
        }
        
        m_spec->mutable_itemsimilarityrecommender()->set_recommendeditemscoreoutputfeaturename(name);
        
        fd->set_allocated_type(outputType);
        output->AddAllocated(fd);
      }
    }
    
    // Dump all this into the protobuf spec.
    for(const auto& item_p : m_isr_data->item_interactions) {
      auto* interactions = m_isr->add_itemitemsimilarities();
      
      uint64_t item = item_p.first;
      
      interactions->set_itemid(item);
      if(m_isr_data->item_shift_values.count(item)) {
        interactions->set_itemscoreadjustment(m_isr_data->item_shift_values[item]);
      } else {
        interactions->set_itemscoreadjustment(0);
      }
      
      auto* item_list = interactions->mutable_similaritemlist();
      item_list->Clear();
      
      for(const auto& inter_p : item_p.second) {
        auto* item_inter = item_list->Add();
        item_inter->set_itemid(inter_p.first);
        item_inter->set_similarityscore(inter_p.second);
      }
    }
    
    if(!m_isr_data->integer_id_values.empty()) {
      auto* integer_vector = m_isr->mutable_itemint64ids()->mutable_vector();
      
      for(int64_t item_id : m_isr_data->integer_id_values) {
        integer_vector->Add(item_id);
      }
    } else if(!m_isr_data->string_id_values.empty()) {
      auto* string_vector = m_isr->mutable_itemstringids()->mutable_vector();
      
      for(std::string item_id : m_isr_data->string_id_values) {
        string_vector->Add(std::move(item_id));
      }
    }
    
    // Finally, construct and validate the model from the generated spec.
    Recommender::constructAndValidateItemSimilarityRecommenderFromSpec(*m_spec);
  }
  
  
  
}
