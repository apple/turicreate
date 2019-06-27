/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>

#include <core/random/random.hpp>

#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/recsys/models.hpp>


using namespace turi;

struct recsys_popularity_test  {
 public:
  void test_popularity() {

    size_t num_observations = 10000;
    size_t num_users = 1000;
    size_t num_items = 10;

    ////////////////////////////////////////////////////////////
    // Build the data
    std::vector<std::vector<flexible_type> > train_data;

    train_data.reserve(num_observations);

    random::seed(0);

    std::set<std::pair<size_t, size_t> > rated_items;

    std::vector<size_t> counts(num_items, 0);
    size_t total_counts = 0;

    // Do one run through with all users and all items;
    for(size_t uid = 0; uid < num_users; ++uid) {
      size_t user = uid;
      size_t item = uid % num_items;
      train_data.push_back( {user, item} );
      rated_items.insert( {user, item} );
      ++counts[item];
      ++total_counts;
    }

    do {
      size_t user = random::fast_uniform<size_t>(0, num_users - 1);
      size_t item = random::fast_uniform<size_t>(0, num_items - 1);

      double accept_prob = 1.0 - double(item) / num_items;

      double r = random::fast_uniform<double>(0.0, 1.0);

      if(r < accept_prob) {
        train_data.push_back( {user, item} );
        rated_items.insert( {user, item} );
        ++counts[item];
        ++total_counts;
      }
    } while(train_data.size() < num_observations);

    sframe data = make_testing_sframe({"user", "item"},
                                      {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                      train_data);


    TS_ASSERT_EQUALS(data.size(), num_observations);

    ////////////////////////////////////////////////////////////
    // Get the model

    std::shared_ptr<recsys::recsys_popularity> model(new recsys::recsys_popularity());

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    model->setup_and_train(data);

    // Instantiate some alternate versions to make sure the save and load work
    std::vector<std::shared_ptr<recsys::recsys_model_base> > all_models =
        {model,
         std::make_shared<recsys::recsys_popularity>(),
         model};

    ////////////////////////////////////////

    save_and_load_object(*all_models[1], *model);

    ////////////////////////////////////////
    // Include the ones generated from the other models.

    {
      std::map<std::string, flexible_type> mf_opts;
      mf_opts["item_id"] = "item";
      mf_opts["user_id"] = "user";
      mf_opts["num_factors"] = 4;
      mf_opts["max_iterations"] = 5;

      auto model_mf = new recsys::recsys_ranking_factorization_model();
      model_mf->init_options(mf_opts);
      model_mf->setup_and_train(data);
      all_models.push_back(model_mf->get_popularity_baseline());
    }

    {
      auto model_item_sim = new recsys::recsys_itemcf();
      model_item_sim->init_options(opts);
      model_item_sim->setup_and_train(data);
      all_models.push_back(model_item_sim->get_popularity_baseline());
    }

    ////////////////////////////////////////////////////////////
    // get a set of predictions; check all the models

    std::vector<std::vector<flexible_type> > pred_loc;

    // Here, this model completely ignores users, so it's sufficient
    // to just test all our operations on a candidate user; in this
    // case "0".
    for(size_t m = 0; m < num_items; ++m)
      pred_loc.push_back( {0, m } );

    sframe pred_sf = make_testing_sframe(
        {"user", "item"},
        {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
        pred_loc);

    // Now make sure both the original model and the saved/loaded
    // model predict how we'd expect.

    for(size_t idx = 0; idx < all_models.size(); ++idx) {

      auto test_model = all_models[idx];

      v2::ml_data pred_ml = test_model->create_ml_data(pred_sf);
      sframe pred_counts_sf = test_model->predict(pred_ml);

      std::vector<double> pred_counts = testing_extract_column<double>(pred_counts_sf.select_column(0));

      for(size_t i = 0; i < num_items; ++i)
        TS_ASSERT_EQUALS(pred_counts[i], double(counts[i]));

      ////////////////////////////////////////////////////////////////////////////////
      // Tests for recommend

      // Create a dataset with half the users

      std::vector<flexible_type> some_users;
      for(size_t i = 0; i < num_users; i += 2)
        some_users.push_back(i);

      std::vector<flexible_type> all_users;
      for(size_t i = 0; i < num_users; ++i)
        all_users.push_back(i);

      for(size_t user_source_index : {0, 1} ) {

        const std::vector<flexible_type>& user_source =
            (user_source_index == 0
             ? all_users
             : some_users);

        std::shared_ptr<sarray<flexible_type> > users_sarray = make_testing_sarray(
          flex_type_enum::INTEGER, user_source);

        sframe users_query = sframe({users_sarray}, {"user"});
        sframe restriction_sf = sframe();
        sframe exclusion_sf = sframe();
        sframe new_observation_sf = sframe();
        sframe new_user_data = sframe();
        sframe new_item_data = sframe();
        bool exclude_training_interactions = false;
        sframe empty_sframe = sframe();

        ////////////////////////////////////////////////////////////
        // All training interactions are properly excluded, when called
        // with all the data
        {

          sframe all_out = test_model->recommend(users_query, num_items);

          std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);

          std::set<std::pair<size_t, size_t> > recommended_pairs;
          for(const auto& row : recommend_out) {
            size_t user = row[0];
            size_t item = row[1];
            ASSERT_EQ(recommended_pairs.count({user, item}), 0);
            ASSERT_EQ(rated_items.count({user, item}), 0);
            recommended_pairs.insert({user, item});
          }

          for(size_t user : user_source) {
            for(size_t item = 0; item < num_items; ++item) {
              if(rated_items.count({user, item}) == 0) {
                ASSERT_EQ(recommended_pairs.count({user, item}), 1);
              }
            }
          }
        }

        ////////////////////////////////////////////////////////////
        // Nothing is excluded when asked to exclude nothing
        {
          exclude_training_interactions = false;

          sframe all_out = test_model->recommend(users_query, num_items, 
                                                 restriction_sf, exclusion_sf, 
                                                 new_observation_sf, 
                                                 new_user_data, new_item_data, 
                                                 exclude_training_interactions);

          std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);

          std::multiset<std::pair<size_t, size_t> > recommended_pairs;
          for(const auto& row : recommend_out) {
            size_t user = row[0];
            size_t item = row[1];
            ASSERT_EQ(recommended_pairs.count({user, item}), 0);
            recommended_pairs.insert({user, item});
          }

          for(size_t user : user_source) {
            for(size_t item = 0; item < num_items; ++item) {
              ASSERT_EQ(recommended_pairs.count({user, item}), 1);
            }
          }
        }

        ////////////////////////////////////////////////////////////
        // Everything is excluded, as expected, when we pass in the
        // training data as an exclusion list.
        {
          sframe exclusion_sf = data;

          sframe all_out = test_model->recommend(users_query, num_items, 
                                                 restriction_sf, exclusion_sf, 
                                                 new_observation_sf, 
                                                 new_user_data, new_item_data, 
                                                 exclude_training_interactions);

          std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);

          std::set<std::pair<size_t, size_t> > recommended_pairs;
          for(const auto& row : recommend_out) {
            size_t user = row[0];
            size_t item = row[1];
            ASSERT_EQ(recommended_pairs.count({user, item}), 0);
            ASSERT_EQ(rated_items.count({user, item}), 0);
            recommended_pairs.insert({user, item});
          }

          for(size_t user : user_source) {
            for(size_t item = 0; item < num_items; ++item) {
              if(rated_items.count({user, item}) == 0) {
                ASSERT_EQ(recommended_pairs.count({user, item}), 1);
              }
            }
          }
        }

        ////////////////////////////////////////////////////////////
        // Nothing is excluded when asked to exclude none of the
        // training data, but still pass in the training data as "new"
        // data
        {
          sframe new_observation_sf = data; 
          exclude_training_interactions = false;

          sframe all_out = test_model->recommend(users_query, num_items, 
                                                 restriction_sf, exclusion_sf, new_observation_sf, 
                                                 new_user_data, new_item_data, 
                                                 exclude_training_interactions);

          std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);

          std::multiset<std::pair<size_t, size_t> > recommended_pairs;
          for(const auto& row : recommend_out) {
            size_t user = row[0];
            size_t item = row[1];
            ASSERT_EQ(recommended_pairs.count({user, item}), 0);
            recommended_pairs.insert({user, item});
          }

          for(size_t user : user_source) {
            for(size_t item = 0; item < num_items; ++item) {
              ASSERT_EQ(recommended_pairs.count({user, item}), 1);
            }
          }
        }

        for(const std::vector<flexible_type>& item_inclusion_list
                : {std::vector<flexible_type>{1, 2, 5, 9},
                  std::vector<flexible_type>{0, 1, 2, 3},
                  std::vector<flexible_type>{0}} ) {

          std::set<size_t> item_inclusion_set(item_inclusion_list.begin(), item_inclusion_list.end());

          std::shared_ptr<sarray<flexible_type> > item_inclusion_sarray = make_testing_sarray(
          flex_type_enum::INTEGER, item_inclusion_list);

          sframe inclusion_sf = sframe({item_inclusion_sarray}, {"item"});

          /// Test recommend when the exclusion list is supplied explicitly
          {

            exclude_training_interactions = true;
            sframe all_out = test_model->recommend(users_query, num_items, 
                                                   inclusion_sf, exclusion_sf, 
                                                   new_observation_sf, 
                                                   new_user_data, new_item_data, 
                                                   exclude_training_interactions);


            std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);

            std::multiset<std::pair<size_t, size_t> > recommended_pairs;
            for(const auto& row : recommend_out) {
              size_t user = row[0];
              size_t item = row[1];
              ASSERT_EQ(item_inclusion_set.count(item), 1);
              ASSERT_EQ(recommended_pairs.count({user, item}), 0);
              ASSERT_EQ(rated_items.count({user, item}), 0);
              recommended_pairs.insert({user, item});
            }

            for(size_t user : user_source) {
              for(size_t item = 0; item < num_items; ++item) {
                if(item_inclusion_set.count(item) == 1) {
                  if(rated_items.count({user, item}) == 0)
                    ASSERT_EQ(recommended_pairs.count({user, item}), 1);
                } else {
                  ASSERT_EQ(recommended_pairs.count({user, item}), 0);
                }
              }
            }
          }

          /// Do it while not excluding the training data
          {
            exclude_training_interactions = false;
            sframe all_out = test_model->recommend(users_query, num_items, 
                                                   inclusion_sf, exclusion_sf, 
                                                   new_observation_sf, 
                                                   new_user_data, new_item_data, 
                                                   exclude_training_interactions);

            std::vector<std::vector<flexible_type> > recommend_out = testing_extract_sframe_data(all_out);
            std::multiset<std::pair<size_t, size_t> > recommended_pairs;
            for(const auto& row : recommend_out) {
              size_t user = row[0];
              size_t item = row[1];
              ASSERT_EQ(item_inclusion_set.count(item), 1);
              ASSERT_EQ(recommended_pairs.count({user, item}), 0);
              recommended_pairs.insert({user, item});
            }

            // Go through all (user, item) pairs and ensure that all recommended pairs
            // are only items in the inclusion set.
            for(size_t user : user_source) {
              for(size_t item = 0; item < num_items; ++item) {
                ASSERT_EQ(recommended_pairs.count({user, item}), item_inclusion_set.count(item));
              }
            }
          }
        }
      }

      ////////////////////////////////////////////////////////////
      // Test the ranking of the items.
      auto user_indexer = test_model->metadata->indexer(test_model->USER_COLUMN_INDEX);
      auto item_indexer = test_model->metadata->indexer(test_model->ITEM_COLUMN_INDEX);

      std::string user_column_name = test_model->metadata->column_name(test_model->USER_COLUMN_INDEX);
      std::string item_column_name = test_model->metadata->column_name(test_model->ITEM_COLUMN_INDEX);

      {
        auto user_sf = sframe({make_testing_sarray(flex_type_enum::INTEGER, {0, 1})},
                              {"user"});
        size_t k = 5;

        // Test the base data through the use of the most flexible option
        std::vector<std::vector<flexible_type> > skip_data =
            {{0, 0},
             {0, 2},
             {1, 2},
             {1, 3} };

        sframe restriction_sf = sframe();
        sframe exclusion_sf = make_testing_sframe(
            {"user", "item"},
            {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
            skip_data);
        sframe new_observation_sf = sframe();
        sframe new_user_data = sframe();
        sframe new_item_data = sframe();
        bool exclude_training_interactions = false;

        sframe ranked_items = test_model->recommend(user_sf, k, 
                                                    restriction_sf, exclusion_sf, 
                                                    new_observation_sf, 
                                                    new_user_data, new_item_data, 
                                                    exclude_training_interactions);

        sframe unindexed_ranked_items = v2::map_from_custom_indexed_sframe(
            { {"user", user_indexer},
              {"item", item_indexer} },
            ranked_items);

        std::vector<std::vector<flexible_type> > res = testing_extract_sframe_data(unindexed_ranked_items);

        // for(size_t user : {0, 1} ) {
        //   for(size_t item = 0; item < num_items; ++item) {
        //     if(rated_items.count({user, item}))
        //       std::cout << "RATED: " << user << "," << item << std::endl;
        //   }
        // }

        // for(std::vector<flexible_type>& row : res) {
        //   std::cout << "res = " << row[0] << "," << row[1] << "," << row[2] << "," << row[3] << ")" << std::endl;
        // }

        std::vector<std::vector<flexible_type> > true_res =
            { {0, 1},
              {0, 3},
              {0, 4},
              {0, 5},
              {0, 6},
              {1, 0},
              {1, 1},
              {1, 4},
              {1, 5},
              {1, 6} };

        TS_ASSERT_EQUALS(res.size(), true_res.size());

        for(size_t i = 0; i < res.size(); ++i) {
          ASSERT_EQ(res[i][0], true_res[i][0]);
          ASSERT_EQ(res[i][1], true_res[i][1]);
        }

        std::vector<double> y_pred = testing_extract_column<double>(ranked_items.select_column("score"));
        TS_ASSERT( (y_pred == std::vector<double>{
                    double(counts[1]),
                    double(counts[3]),
                    double(counts[4]),
                    double(counts[5]),
                    double(counts[6]),
                    double(counts[0]),
                    double(counts[1]),
                    double(counts[4]),
                    double(counts[5]),
                    double(counts[6])}) );
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_recsys_popularity_test, recsys_popularity_test)
BOOST_AUTO_TEST_CASE(test_popularity) {
  recsys_popularity_test::test_popularity();
}
BOOST_AUTO_TEST_SUITE_END()
