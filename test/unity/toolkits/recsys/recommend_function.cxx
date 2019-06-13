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

struct recsys_recommend  {
 public:
  void test_user_item_inclusions() {

    // Build a simple dataset

    sframe obs_data = make_integer_testing_sframe( {"user", "item", "side", "target"},
                                                   { {10,20, 1, 3},
                                                     {10,21, 2, 2},
                                                     {10,22, 3, 1},
                                                     {11,20, 4, 3},
                                                     {11,21, 5, 2},
                                                     {11,25, 6, 1} } ); 
    
    auto model = new recsys::recsys_popularity();

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "target";
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    model->setup_and_train(obs_data);


    sframe inc_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {10, 20},
                                                     {11, 20},
                                                     {10, 21},
                                                     {11, 21} } ); 


    sframe res_back = model->recommend(sframe(), 10, inc_data, sframe(), sframe(), sframe(), sframe(), false);

    std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
    ASSERT_EQ(res.size(), 4); 

    auto true_0 = flex_list{10,20,3,1};
    auto true_1 = flex_list{10,21,2,2};
    auto true_2 = flex_list{11,20,3,1};
    auto true_3 = flex_list{11,21,2,2};
    
    // Comes as user/item/score/rank
    ASSERT_TRUE(res[0] == true_0);
    ASSERT_TRUE(res[1] == true_1);
    ASSERT_TRUE(res[2] == true_2);
    ASSERT_TRUE(res[3] == true_3);
  }


  void test_item_inclusions_with_original_data() {

    // Build a simple dataset

    sframe obs_data = make_integer_testing_sframe( {"user", "item", "side", "target"},
                                                   { {10,20, 1, 3},
                                                     {10,21, 2, 2},
                                                     {10,22, 3, 1},
                                                     {11,20, 4, 3},
                                                     {11,21, 5, 2},
                                                     {11,25, 6, 1} } ); 
    
    auto model = new recsys::recsys_popularity();

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "target";
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    model->setup_and_train(obs_data);

    sframe inc_data = make_integer_testing_sframe( {"item"},
                                                     { {20},
                                                       {21} } ); 
      
    sframe inc_data_2 = make_integer_testing_sframe( {"user", "item"},
                                                     { {10,20},
                                                       {10,25},
                                                       {11,21},
                                                       {11,22} });

    sframe user_sf_orig = make_integer_testing_sframe( {"user"},
                                                       { {10},
                                                         {11} });

    sframe user_sf_more = make_integer_testing_sframe( {"user"},
                                                       { {10},
                                                         {11},
                                                         {30} });
    
    
    sframe user_sf = make_integer_testing_sframe( {"user"},
                                                  { {30},
                                                    {31} });
    


    {
      sframe res_back = model->recommend(user_sf, 10, inc_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 4); 

      auto true_0 = flex_list{30,20,3,1};
      auto true_1 = flex_list{30,21,2,2};
      auto true_2 = flex_list{31,20,3,1};
      auto true_3 = flex_list{31,21,2,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }
   
    {
      // Same one, but with exclude_training_interactions set to 0
      sframe res_back = model->recommend(user_sf, 10, inc_data, sframe(), sframe(), sframe(), sframe(), false);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 4); 

      auto true_0 = flex_list{30,20,3,1};
      auto true_1 = flex_list{30,21,2,2};
      auto true_2 = flex_list{31,20,3,1};
      auto true_3 = flex_list{31,21,2,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }

    {
      // now one with per-user item inclusions
      sframe res_back = model->recommend(sframe(), 10, inc_data_2);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 2); 

      auto true_0 = flex_list{10,25,1,1};
      auto true_1 = flex_list{11,22,1,1};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
    }

    {
      // now a similar one but with users specified
      sframe res_back = model->recommend(user_sf_orig, 10, inc_data_2);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 2); 

      auto true_0 = flex_list{10,25,1,1};
      auto true_1 = flex_list{11,22,1,1};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
    }


    {

      // now a similar one but with users specified; plus a user not
      // in inc_data_2, which should be ignored.
      sframe res_back = model->recommend(user_sf_more, 10, inc_data_2);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 2); 

      auto true_0 = flex_list{10,25,1,1};
      auto true_1 = flex_list{11,22,1,1};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
    }
    
    {
      // now a similar one but without the training exclusions
      
      sframe res_back = model->recommend(sframe(), 10, inc_data_2, sframe(),
                                         sframe(), sframe(), sframe(), false);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 4); 

      auto true_0 = flex_list{10,20,3,1};
      auto true_1 = flex_list{10,25,1,2};
      auto true_2 = flex_list{11,21,2,1};
      auto true_3 = flex_list{11,22,1,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }

    {
      // now a similar one but with users specified
      sframe res_back = model->recommend(user_sf_orig, 10, inc_data_2, sframe(),
                                         sframe(), sframe(), sframe(), false);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 4);

      auto true_0 = flex_list{10,20,3,1};
      auto true_1 = flex_list{10,25,1,2};
      auto true_2 = flex_list{11,21,2,1};
      auto true_3 = flex_list{11,22,1,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }

    {
      // now a similar one but with users specified; plus a user not
      // in inc_data_2, which should be ignored.
      sframe res_back = model->recommend(user_sf_more, 10, inc_data_2, sframe(),
                                         sframe(), sframe(), sframe(), false);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
    
      ASSERT_EQ(res.size(), 4);

      auto true_0 = flex_list{10,20,3,1};
      auto true_1 = flex_list{10,25,1,2};
      auto true_2 = flex_list{11,21,2,1};
      auto true_3 = flex_list{11,22,1,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }
    

    
    // Now make sure items marked for exclusion are indeed excluded.
    
    sframe exc_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {30, 20} } );

    {
      sframe res_back = model->recommend(user_sf, 10, inc_data, exc_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 3); 

      // 1,2,3 above; 0 should be excluded.
      auto true_0 = flex_list{30,21,2,1};
      auto true_1 = flex_list{31,20,3,1};
      auto true_2 = flex_list{31,21,2,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
    }


    // Now make sure items included as the new data are also excluded
    sframe new_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {31, 21} } );
    
    {
      sframe res_back = model->recommend(user_sf, 10, inc_data, exc_data, new_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 2); 

      // 1,2,3 above; 0 should be excluded.
      auto true_0 = flex_list{30,21,2,1};
      auto true_1 = flex_list{31,20,3,1};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
    }
  }


  void test_user_item_inclusions_with_original_data() {

    // Build a simple dataset

    sframe obs_data = make_integer_testing_sframe( {"user", "item", "side", "target"},
                                                   { {10,20, 1, 3},
                                                     {10,21, 2, 2},
                                                     {10,22, 3, 1},
                                                     {11,20, 4, 3},
                                                     {11,21, 5, 2},
                                                     {11,25, 6, 1} } ); 
    
    auto model = new recsys::recsys_popularity();

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "target";
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    model->setup_and_train(obs_data);

    sframe inc_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {30, 20},
                                                     {30, 21},
                                                     {31, 20},
                                                     {31, 22} } );


    sframe user_sf = make_integer_testing_sframe( {"user"},
                                                  { {30},
                                                    {31} });
    


    {
      sframe res_back = model->recommend(user_sf, 10, inc_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 4); 

      auto true_0 = flex_list{30,20,3,1};
      auto true_1 = flex_list{30,21,2,2};
      auto true_2 = flex_list{31,20,3,1};
      auto true_3 = flex_list{31,22,1,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
      ASSERT_TRUE(res[3] == true_3);
    }

    // Now make sure items marked for exclusion are indeed excluded.
    
    sframe exc_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {30, 20} } );

    {
      sframe res_back = model->recommend(user_sf, 10, inc_data, exc_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 3); 

      // 1,2,3 above; 0 should be excluded.
      auto true_0 = flex_list{30,21,2,1};
      auto true_1 = flex_list{31,20,3,1};
      auto true_2 = flex_list{31,22,1,2};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
      ASSERT_TRUE(res[2] == true_2);
    }


    // Now make sure items included as the new data are also excluded
    sframe new_data = make_integer_testing_sframe( {"user", "item"},
                                                   { {31, 22} } );
    
    {
      sframe res_back = model->recommend(user_sf, 10, inc_data, exc_data, new_data);
      
      std::vector<flex_list> res = testing_extract_sframe_data(res_back); 
    
      ASSERT_EQ(res.size(), 2); 

      // 1,2,3 above; 0 should be excluded.
      auto true_0 = flex_list{30,21,2,1};
      auto true_1 = flex_list{31,20,3,1};
    
      // Comes as user/item/score/rank
      ASSERT_TRUE(res[0] == true_0);
      ASSERT_TRUE(res[1] == true_1);
    }
  }


  void test_side_columns_used() { 

    // The side column exactly predicts the target column; 
    sframe obs_data = make_testing_sframe( {"user", "item", "side", "target"},
                                           { {10, 20, 1,  1},
                                             {10, 21, 3,  3},
                                             {10, 22, 8,  8},
                                             {11, 20, 5,  5},
                                             {11, 21, 20, 20},
                                             {11, 22, 2,  2},
                                             {12, 20, 1,  1},
                                             {12, 21, 5,  5},
                                             {12, 22, 12, 12},
                                             {13, 20, 2,  2},
                                             {13, 21, 10, 10},
                                                 // This one is 23, so each user has one unrated item
                                             {13, 23, 10, 10}, 
                                                         
                                             {10, 20, -1,  -1},
                                             {10, 21, -3,  -3},
                                             {10, 22, -8,  -8},
                                             {11, 20, -5,  -5},
                                             {11, 21, -20, -20},
                                             {11, 22, -2,  -2},
                                             {12, 20, -1,  -1},
                                             {12, 21, -5,  -5},
                                             {12, 22, -12, -12},
                                             {13, 20, -2,  -2},
                                             {13, 21, -10, -10},
                                                 // This one is 23, so each user has one unrated item
                                             {13, 23, -10, -10} } );


    auto model = new recsys::recsys_factorization_model();

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "target";
    opts["num_factors"] = 0;
    opts["max_iterations"] = 1000;
    opts["linear_regularization"] = 0;
    opts["regularization"] = 0;
    
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model
    parallel_for(size_t(0), size_t(16), [&](size_t i) {
        if(i == 0) 
          model->setup_and_train(obs_data);
      }); 

    {
      // Now there is no side data.  The predicted scores should
      // basically be the average, which is 0. 
      sframe res_back = model->recommend(sframe(), 1);

      std::vector<double> scores = testing_extract_column<double>(res_back.select_column("score")); 

      // Make sure each of these equals zero
      for(double v : scores) {
        TS_ASSERT_DELTA(0.0, v, 0.05);
      }
    }

    // Now with side data. 
    // The side column exactly predicts the target column;
    {
      std::vector<double> rv = {1.5, -5, 5, -2}; 
      sframe query_data = make_testing_sframe( {"user", "side"},
                                               { {10,  rv[0]},
                                                 {11,  rv[1]},
                                                 {12,  rv[2]},
                                                 {13,  rv[3]} });

      sframe res_back = model->recommend(query_data, 1);

      std::vector<double> scores = testing_extract_column<double>(res_back.select_column("score")); 

      ASSERT_EQ(scores.size(), 4); 
    
      // Make sure each of these equals zero
      for(size_t i = 0; i < scores.size(); ++i)
        TS_ASSERT_DELTA(scores[i], rv[i], 0.05);
    }
  }

  template <typename Model> 
  void _run_test_diversity() {

    // Build a simple dataset
    
    sframe data = make_random_sframe(1000, "CC");

    data.set_column_name(0, "user");
    data.set_column_name(1, "item");

    std::unique_ptr<recsys::recsys_model_base> model(new Model); 

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "";
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    model->setup_and_train(data);

    for(size_t _k = 1; _k < 10; ++_k) {

      size_t k = (_k < 5) ? _k : 5 + 3*(_k - 5);
      
      sframe res_back = model->recommend(sframe(), k, sframe(), sframe(), sframe(), sframe(), sframe(),
                                         false, /*diversity=*/ 1, /*random_seed=*/0);

      sframe res_back_2 = model->recommend(sframe(), k, sframe(), sframe(), sframe(), sframe(), sframe(),
                                           false, /*diversity=*/ 1, /*random_seed=*/1);

      sframe res_back_3 = model->recommend(sframe(), k, sframe(), sframe(), sframe(), sframe(), sframe(),
                                           false, /*diversity=*/ 2, /*random_seed=*/0);
    
      std::vector<flex_list> res = testing_extract_sframe_data(res_back);
      std::vector<flex_list> res_2 = testing_extract_sframe_data(res_back_2);
      std::vector<flex_list> res_3 = testing_extract_sframe_data(res_back_3);

      ASSERT_EQ(res.size(), res_2.size());
      ASSERT_EQ(res.size(), res_3.size());

      bool all_equal_1 = true;
      bool all_equal_2 = true;
    
      for(size_t i = 0; i < res.size(); ++i) {
        if(res[i] != res_2[i]) {
          all_equal_1 = false;
        }

        if(res[i] != res_3[i]) {
          all_equal_2 = false;
        }
      }
    
      ASSERT_FALSE(all_equal_1);
      ASSERT_FALSE(all_equal_2);
    }
  }

  void test_diversity_popularity() {
    _run_test_diversity<recsys::recsys_popularity>();
  }

  void test_diversity_mf() {
    _run_test_diversity<recsys::recsys_ranking_factorization_model>();
  }

  void test_diversity_itemcf() {
    _run_test_diversity<recsys::recsys_itemcf>();
  }
  
}; 


  

BOOST_FIXTURE_TEST_SUITE(_recsys_recommend, recsys_recommend)
BOOST_AUTO_TEST_CASE(test_user_item_inclusions) {
  recsys_recommend::test_user_item_inclusions();
}
BOOST_AUTO_TEST_CASE(test_item_inclusions_with_original_data) {
  recsys_recommend::test_item_inclusions_with_original_data();
}
BOOST_AUTO_TEST_CASE(test_user_item_inclusions_with_original_data) {
  recsys_recommend::test_user_item_inclusions_with_original_data();
}
BOOST_AUTO_TEST_CASE(test_side_columns_used) {
  recsys_recommend::test_side_columns_used();
}
BOOST_AUTO_TEST_CASE(test_diversity_popularity) {
  recsys_recommend::test_diversity_popularity();
}
BOOST_AUTO_TEST_CASE(test_diversity_mf) {
  recsys_recommend::test_diversity_mf();
}
BOOST_AUTO_TEST_CASE(test_diversity_itemcf) {
  recsys_recommend::test_diversity_itemcf();
}
BOOST_AUTO_TEST_SUITE_END()
