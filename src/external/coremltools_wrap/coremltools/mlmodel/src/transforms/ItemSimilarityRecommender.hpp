//
//  ItemSimilarityRecommender.hpp
//  CoreML_framework
//
//  Created by Hoyt Koepke on 1/29/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#ifndef ItemSimilarityRecommender_hpp
#define ItemSimilarityRecommender_hpp

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../Model.hpp"

namespace CoreML {
        // Forward declare structs to abstract way storage layouts.
    namespace Specification {
        class ItemSimilarityRecommender;
    }

    namespace Recommender {
        class _ItemSimilarityRecommenderData;
    }

    class ItemSimilarityRecommender : public Model {
    public:
        /**  Create an item similarity recommender that scores items in a collection
         *   and outputs the ones most recommended.
         *
         */
        ItemSimilarityRecommender(const std::string& description = "");

        /** Set the column name for the Item Id output.  This is a sequence of at most K items,
         *  ordered from highest scored to lowest scored.
         */
        void setRecommendedItemIdOutputName(const std::string& recommendedItemIdOutputName);

        /** Set the column name for the Item score output.  This is a dictionary of Item ID
         *  to score.
         */
        void setRecommendedItemScoreOutputName(const std::string& recommendedItemScoresOutputName);

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
        void addItemItemInteraction(size_t reference_item_id, size_t linked_item_id, double link_value, bool symmetric = false);

        /** Sets the adjustment value of this item that is applied to the user's
         *  rating of the item.  This may be used to adjust for biases in the item
         *  values relative to the user's score.
         */
        void setItemShiftValue(size_t item_id, double value);

        /** Sets the name of the input data feature.  If include_scores is True, than the
         *  expected input is a dictionary of item to score.  If it is false, it's a
         *  MLMultiArray of item ids.
         */
        void setItemDataInputFeatureName(const std::string& name, bool include_scores);

        /** Sets the name of the column that dictates how many recommended
         *  items are returned by the model.
         */
        void setNumRecommendationsInputFeatureName(const std::string& name);

        /** Sets the name of the column that allows the user to restrict recommended items to
         *  a specific set.
         */
        void setItemRestrictionInputFeatureName(const std::string& name);

        /** Sets the name of the column that allows the user to exclude items from recommendation.
         *  If given, this overrides the default behavior, which is to exclude all the items
         *  in the input item list.  Specifying this parameter and then passing in an empty
         *  array of item ids causes the recommendations to come from all available items.
         */
        void setItemExclusionInputFeatureName(const std::string& name);

        /**  Sets a mapping of the integer values of the items from their index values above
         *   to an integer ID.  Input items are assumed to be this ID, and outputs are given as such
         *   an ID.  If neither setItemIntegerList or setItemStringList is called, the item IDs
         *   are assumed to match the indices above.
         */
        void setItemIntegerList(const std::vector<int64_t>& integer_items);

        /**  Sets a mapping of the integer indices of the items from their index values above
         *   to a string ID.  Input items are assumed to be this ID, and outputs are given as such
         *   an ID.  If neither setItemIntegerList or setItemStringList is called, the item IDs
         *   are assumed to match the indices above.
         */
        void setItemStringList(const std::vector<std::string>& string_items);

        /** Saves and validates the current model.
         */
        void finish();

    private:
        std::shared_ptr<Recommender::_ItemSimilarityRecommenderData> m_isr_data;
        bool use_dictionary_input = false;

        CoreML::Specification::ItemSimilarityRecommender* m_isr;
    };


}

#endif /* ItemSimilarityRecommender_hpp */
