/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <vector>
#include <string>
#include <random/random.hpp>
#include <unity/toolkits/recsys/models.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>

#include <Eigen/Core>

using namespace turi;

struct recsys_itemcf_test  {
 public:

  void run_itemcf(const std::string& similarity_type,
                  size_t num_observations, size_t num_users,
                  size_t num_items,
                  const std::string& training_method,
                  size_t max_item_neighborhood_size=0) {

    ////////////////////////////////////////////////////////////
    // Build the data
    std::vector<std::vector<flexible_type> > train_data;

    train_data.reserve(num_observations);

    random::seed(0);

    std::vector<size_t> counts(num_items, 0);

    do {
      size_t user = random::fast_uniform<size_t>(0, num_users - 1);
      size_t item = random::fast_uniform<size_t>(0, num_items - 1);

      double accept_prob = 1.0 - double(item) / num_items;

      double r = random::fast_uniform<double>(0.0, 1.0);

      if(r < accept_prob) {
        double rating = random::fast_uniform<double>(1.0, 5.0);
        train_data.push_back( {std::to_string(user), std::to_string(item), rating} );
        ++counts[item];
      }
    } while(train_data.size() < num_observations);

    sframe data = make_testing_sframe({"user", "item", "rating"},
                                      {flex_type_enum::STRING, flex_type_enum::STRING, flex_type_enum::FLOAT},
                                      train_data);

    DASSERT_EQ(data.size(), num_observations);

    ////////////////////////////////////////////////////////////
    // Get the model
    {
      auto model = std::make_shared<recsys::recsys_itemcf>();

      ////////////////////////////////////////////////////////////
      // Set the options

      std::map<std::string, flexible_type> opts;
      opts["item_id"] = "item";
      opts["user_id"] = "user";
      opts["target"] = "rating";
      opts["similarity_type"] = similarity_type;
      opts["training_method"] = training_method;
      model->init_options(opts);

      ////////////////////////////////////////////////////////////
      // Train the model

      model->setup_and_train(data);

      // Instantiate some alternate versions to make sure the save and load work
      std::vector<std::shared_ptr<recsys::recsys_model_base> > all_models =
          {model, std::make_shared<recsys::recsys_itemcf>(), model};

      save_and_load_object(*all_models[1], *model);

      sframe pred = model->predict(model->create_ml_data(data));

      // Test adding new observation data
      std::vector<std::vector<flexible_type> > new_observation_data;
      new_observation_data.reserve(4);
      new_observation_data.push_back( {"my new user", "0", 1.0} );
      new_observation_data.push_back( {"my new user", "1", 1.0} );
      new_observation_data.push_back( {"my new user", "2", 1.0} );
      new_observation_data.push_back( {"my new user", "3", 1.0} );


      sframe new_observations = make_testing_sframe({"user", "item", "rating"},
                                                    {flex_type_enum::STRING,
                                                    flex_type_enum::STRING,
                                                    flex_type_enum::FLOAT},
                                                    new_observation_data);

      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, {"0", "1", "2", "3"});
      sframe users_query = sframe({users}, {"user"});

      v2::ml_data new_obs_mldata = model->create_ml_data(new_observations);
      auto item_restriction_list = std::vector<size_t>();
      size_t topk = 7;
      sframe restriction_sf = sframe();
      sframe exclusion_sf = sframe();
      sframe new_user_data = sframe();
      sframe new_item_data = sframe();
      bool exclude_training_interactions = false;

      sframe recs = model->recommend(users_query, topk,
                                     restriction_sf,
                                     exclusion_sf,
                                     new_observations,
                                     new_user_data, new_item_data,
                                     exclude_training_interactions);


      DASSERT_EQ(recs.num_rows(), new_observations.num_rows() * topk);
    }

    ////////////////////////////////////////////////////////////
    // Retrain the model without setting target_column
    {
      auto model = std::make_shared<recsys::recsys_itemcf>();

      ////////////////////////////////////////////////////////////
      // Set the options

      std::map<std::string, flexible_type> opts;
      opts["item_id"] = "item";
      opts["user_id"] = "user";
      opts["target"] = "";
      opts["similarity_type"] = similarity_type;
      opts["training_method"] = training_method;
      model->init_options(opts);

      ////////////////////////////////////////////////////////////
      // Train the model

      model->setup_and_train(data);

      // Instantiate some alternate versions to make sure the save and load work
      std::vector<std::shared_ptr<recsys::recsys_model_base> > all_models =
          {model, std::make_shared<recsys::recsys_itemcf>(), model};

      ////////////////////////////////////////

      save_and_load_object(*all_models[1], *model);

      sframe pred = model->predict(model->create_ml_data(data));

    }

  }

  void test_itemcf_jaccard() {
    run_itemcf("jaccard", 50, 10, 10, "auto");
    run_itemcf("jaccard", 50, 10, 10, "dense");
    run_itemcf("jaccard", 50, 10, 10, "sparse");
    run_itemcf("jaccard", 50, 10, 10, "nn");
    run_itemcf("jaccard", 50, 10, 10, "nn:dense");
    run_itemcf("jaccard", 50, 10, 10, "nn:sparse");
  }

  // These are very slow on clang. Temporarily disabling.
  void test_itemcf_cosine() {
    run_itemcf("cosine", 50000, 1000, 100, "auto");
  }

  void test_itemcf_jaccard_2() {
    run_itemcf("jaccard", 50000, 1000, 100, "auto");
  }

  void test_itemcf_pearson_2() {
    // run_itemcf("pearson", 50000, 1000, 100, "auto");
  }

  // reduce the time of runing tests.
  // only run it when necessary

  // void test_large_dataset() {
  //    run_itemcf("jaccard", 2e5, 1e4, 2e4, "sgraph");
  // }


  // Second example for testing distance computations
  // The observed data is as follows, where each row is a "user"
  // and each column is an "item" and each entry is the response.
  //
  //      A    B    C    D
  // 0  1.0  0.3  0.5  0.0
  // 1  0.0  0.5  0.6  0.0
  // 2  0.0  0.0  1.0  1.0
  // 3  0.1  0.0  0.0  1.5
  //


  /** Test similarity computation when target column is not specified.
   */
  void similarity_computation_without_rating(const std::string& training_method) {
    const double DELTA = .0000001;

    std::vector<std::vector<flexible_type>> raw_data = {
      {"0", "A", 1.0}, {"0", "B", .3}, {"0", "C", .5},
      {"1", "B", .5}, {"1", "C", .6},
      {"2", "C", 1.}, {"2", "D", 1.},
      {"3", "A", .1}, {"3", "D", 1.5}};

    sframe data = make_testing_sframe({"user", "item", "rating"},
                                      {flex_type_enum::STRING, flex_type_enum::STRING,
                                      flex_type_enum::FLOAT},
                                      raw_data);

    DASSERT_EQ(data.num_rows(), 9);


    auto model = std::make_shared<recsys::recsys_itemcf>();

    /////////////////////////////////////////////////////////////////////////////
    // Test jaccard
    //

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "";
    opts["similarity_type"] = "jaccard";
    opts["training_method"] = training_method;
    opts["max_item_neighborhood_size"] = 4;
    model->init_options(opts);
    model->setup_and_train(data);

    Eigen::MatrixXd ans(4,4);

    double a_b = 1./3.;
    double a_c = 1./4.;
    double a_d = 1./3.;
    double b_c = 2./3.;
    double b_d = 0.;
    double c_d = 1./4.;

    ans << 1.,  a_b, a_c, a_d,
        0.0, 1.,  b_c, b_d,
        0.0, 0.0, 1.,  c_d,
        0.0, 0.0, 0.0, 1.;

    //
    // test getting item neighbors
    {
      std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
      std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};

      std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);

      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
      sframe users_query = sframe({users}, {"user"});

      auto ret_item_neighbor = model->get_similar_items(all_items, 4);

      auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

      std::vector<std::vector<flexible_type>> rows;
      ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                          rows);

      std::vector<std::vector<flexible_type>> truth;
      truth.reserve(ret_item_neighbor.size());

      for (size_t i = 0; i < 4; i++) {
        std::vector<std::pair<size_t, double> > item_pair_list;
        for (size_t j = 0; j < 4; j++) {
          if (i == j) continue;
          if (i < j)
            item_pair_list.push_back(std::make_pair(j, ans(i,j)));
          else
            item_pair_list.push_back(std::make_pair(j, ans(j,i)));
        }
        std::sort(item_pair_list.begin(), item_pair_list.end(),
                  [](const std::pair<size_t, double>& a,
                     const std::pair<size_t, double>& b)
                  {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
        size_t r = 0;
        for (size_t j = 0; j < item_pair_list.size(); j++) {
          if (item_pair_list[j].second == 0) continue;
          truth.push_back({all_items_vec[i],
                           all_items_vec[item_pair_list[j].first],
                           item_pair_list[j].second,
                           ++r});
        }
      }

      for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){
        logprogress_stream << "returned: " ;
        for (size_t j = 0; j < 4; ++j)
          logprogress_stream << rows[i][j];
        logprogress_stream << "\n";
        logprogress_stream << "truth: " ;
        for (size_t j = 0; j < 4; ++j)
          logprogress_stream << truth[i][j];
        logprogress_stream << "\n";

        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
        flexible_type score = rows[i][2];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];
        DASSERT_EQ(item, truth[i][0]);
        DASSERT_EQ(neighbor, truth[i][1]);
        TS_ASSERT_DELTA(score, truth[i][2], DELTA);
        DASSERT_EQ(rank, truth[i][3]);
      }

      auto all_item_neighbors = model->get_similar_items(nullptr);

      DASSERT_EQ(all_item_neighbors.num_rows(), ret_item_neighbor.num_rows());

      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d + b_d + c_d)/3.},
        {1, 0, (a_b + a_c) / 2.},
        {1, 3, (b_d + c_d) / 2.},
        {2, 0, (a_c + a_d) / 2.},
        {2, 1, (b_c + b_d) /2.},
        {3, 1, (a_b + b_d) / 2.},
        {3, 2, (a_c + c_d) / 2}
      };

      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());

      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }

    }



    /////////////////////////////////////////////////////////////////////////
    // Test cosine similarity
    //

    model = std::make_shared<recsys::recsys_itemcf>();
    opts["similarity_type"] = "cosine";
    opts["target"] = "";
    model->init_options(opts);
    model->setup_and_train(data);

    a_b = (1.) / std::sqrt(2.) / std::sqrt(2.);
    a_c = (1.) / std::sqrt(2.) / std::sqrt(3.);
    a_d = (1.) / std::sqrt(2.) / std::sqrt(2.);
    b_c = (2.) / std::sqrt(2.) / std::sqrt(3.);
    b_d = 0.0;
    c_d = 1.0 / std::sqrt(3.) / std::sqrt(2.);

  ans << 1. ,  a_b , a_c , a_d
      ,0.0 , 1. ,  b_c , b_d
      ,0.0 , 0.0 , 1. ,  c_d 
      ,0.0 , 0.0 , 0.0 , 1.;

    // test getting item neighbors
    {
      std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
      std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};
      std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);
      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
      sframe users_query = sframe({users}, {"user"});



      auto ret_item_neighbor = model->get_similar_items(all_items, 4);
      auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

      std::vector<std::vector<flexible_type>> rows;
      ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                          rows);

      std::vector<std::vector<flexible_type>> truth;
      truth.reserve(ret_item_neighbor.size());

      for (size_t i = 0; i < 4; i++) {
        std::vector<std::pair<size_t, double> > item_pair_list;
        for (size_t j = 0; j < 4; j++) {
          if (i == j) continue;
          if (i < j)
            item_pair_list.push_back(std::make_pair(j, ans(i,j)));
          else
            item_pair_list.push_back(std::make_pair(j, ans(j,i)));
        }
        std::sort(item_pair_list.begin(), item_pair_list.end(),
                  [](const std::pair<size_t, double>& a,
                     const std::pair<size_t, double>& b)
                  {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
        size_t r = 0;
        for (size_t j = 0; j < item_pair_list.size(); j++) {
          if (item_pair_list[j].second == 0) continue;
          truth.push_back({all_items_vec[i],
                           all_items_vec[item_pair_list[j].first],
                           item_pair_list[j].second,
                           ++r});
        }
      }

      for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){

        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
        flexible_type score = rows[i][2];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];

        DASSERT_EQ(item, truth[i][0]);
        DASSERT_EQ(neighbor, truth[i][1]);
        TS_ASSERT_DELTA(score, truth[i][2], DELTA);
        DASSERT_EQ(rank, truth[i][3]);

      }

      // test all items
      auto all_item_neighbors = model->get_similar_items(nullptr);

      DASSERT_EQ(all_item_neighbors.num_rows(), ret_item_neighbor.num_rows());

      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d + b_d + c_d)/3.}, {1, 0, (a_b + a_c) / 2.},
        {1, 3, (b_d + c_d) / 2.}, {2, 0, (a_c + a_d) / 2.},
        {2, 1, (b_c + b_d) /2.}, {3,1, (a_b + b_d) / 2.},
        {3,2, (a_c + c_d) / 2}
      };

      // indexed_items is {0,1,2,3}
      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());
      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }

    }


    /////////////////////////////////////////////////////
    // Test pearson similarity
    model = std::make_shared<recsys::recsys_itemcf>();
    opts["similarity_type"] = "pearson";
    opts["target"] = "";
    model->init_options(opts);
    model->setup_and_train(data);

    // For pearson, when the target column is not specified, all the target
    // values are 1. So the variances of items are all 0s and the similarities
    // between items are also 0s.
    a_b = 0.;
    a_c = 0.;
    a_d = 0.;
    b_c = 0.;
    b_d = 0.;
    c_d = 0.;

    ans << 1.,  a_b, a_c, a_d,
        0.0, 1.,  b_c, b_d,
        0.0, 0.0, 1.,  c_d,
        0.0, 0.0, 0.0, 1.;

    // test getting item neighbors
    {
      std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};

      std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);

      std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
      sframe users_query = sframe({users}, {"user"});

      logprogress_stream << "Num rows on user_query: "
                         << users_query.num_rows() << std::endl;


      auto ret_item_neighbor = model->get_similar_items(all_items, 4);

      auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

      std::vector<std::vector<flexible_type>> rows;
      ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                          rows);

      std::vector<std::vector<flexible_type>> truth;
      truth.reserve(ret_item_neighbor.size());

      for (size_t i = 0; i < 4; i++) {
        std::vector<std::pair<size_t, double> > item_pair_list;
        for (size_t j = 0; j < 4; j++) {
          if (i == j) continue;
          if (i < j)
            item_pair_list.push_back(std::make_pair(j, ans(i,j)));
          else
            item_pair_list.push_back(std::make_pair(j, ans(j,i)));
        }
        std::sort(item_pair_list.begin(), item_pair_list.end(),
                  [](const std::pair<size_t, double>& a,
                     const std::pair<size_t, double>& b)
                  {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
        size_t r = 0;
        for (size_t j = 0; j < item_pair_list.size(); j++) {
          if (item_pair_list[j].second == 0) continue;
          truth.push_back({all_items_vec[i],
                           all_items_vec[item_pair_list[j].first],
                           item_pair_list[j].second, ++r});
        }
      }

      for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){

        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
        flexible_type score = rows[i][2];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];

        DASSERT_EQ(item, truth[i][0]);
        DASSERT_EQ(neighbor, truth[i][1]);
        TS_ASSERT_DELTA(score, truth[i][2], DELTA);
        DASSERT_EQ(rank, truth[i][3]);
      }

      // test new items
      std::vector<flexible_type> new_items;

      auto new_item_neighbors = model->get_similar_items(
          make_testing_sarray(flex_type_enum::INTEGER, new_items)
          );

      DASSERT_EQ(new_item_neighbors.num_rows(), ret_item_neighbor.num_rows());

      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d + b_d + c_d)/3.},
        {1, 0, (a_b + a_c) / 2.},
        {1, 3, (b_d + c_d) / 2.},
        {2, 0, (a_c + a_d) / 2.},
        {2, 1, (b_c + b_d) /2.},
        {3,1, (a_b + b_d) / 2.},
        {3,2, (a_c + c_d) / 2}
      };

      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());

      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }
    }

  }

  /** Test similarity computations and recommendations/preictions
   */
  void similarity_computation_with_rating(const std::string& training_method) {

    const double DELTA = .0000001;
    std::vector<std::vector<flexible_type>> raw_data = {
      {"0", "A", 1.0}, {"0", "B", .3}, {"0", "C", .5},
      {"1", "B", .5}, {"1", "C", .6},
      {"2", "C", 1.}, {"2", "D", 1.},
      {"3", "A", .1}, {"3", "D", 1.5}};

    sframe data = make_testing_sframe({"user", "item", "rating"},
                                      {flex_type_enum::STRING, flex_type_enum::STRING,
                                      flex_type_enum::FLOAT},
                                      raw_data);

    DASSERT_EQ(data.num_rows(), 9);

    ////////////////////////////////////////////////////////////////////
    //  Test pearson
    auto model = std::make_shared<recsys::recsys_itemcf>();
    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "rating";
    opts["similarity_type"] = "pearson";
    opts["training_method"] = training_method;
    opts["max_item_neighborhood_size"] = 4;
    model->init_options(opts);
    model->setup_and_train(data);

    double a_mean = (1. + 0.1) / 2;
    double b_mean = (0.3 + 0.5) / 2;
    double c_mean = (0.5 + 0.6 + 1.0) / 3;
    double d_mean = (1. + 1.5) / 2;

    double a_var = (1. - a_mean) * (1. - a_mean) + (0.1 - a_mean) * (0.1 - a_mean);
    double b_var = (0.3 - b_mean) * (0.3 - b_mean) + (0.5 - b_mean) * (0.5 - b_mean);
    double c_var = (0.5 - c_mean) * (0.5 - c_mean) + (0.6 - c_mean) * (0.6 - c_mean) + (1. - c_mean) * (1. - c_mean);
    double d_var = (1. - d_mean) *  (1. - d_mean) + (1.5 - d_mean) * (1.5 - d_mean);

    double a_b = (1. - a_mean) * (.3 - b_mean) / std::sqrt(a_var) / std::sqrt(b_var);
    double a_c = (1. - a_mean) * (.5 - c_mean) / std::sqrt(a_var) / std::sqrt(c_var);
    double a_d = (.1 - a_mean) * (1.5 - d_mean) / std::sqrt(a_var) / std::sqrt(d_var);
    double b_c = ((.3 - b_mean) * (.5 - c_mean) + (.5 - b_mean) * (.6 - c_mean))
        / std::sqrt(b_var)
        / std::sqrt(c_var);
    double b_d = 0.0;
    double c_d = (1. - c_mean) * (1. - d_mean) / std::sqrt(c_var) / std::sqrt(d_var);


    Eigen::MatrixXd ans(4,4);
    ans << 1. , a_b, a_c, a_d,
        0.0, 1. , b_c, b_d,
        0.0, 0.0, 1.0, c_d,
        0.0, 0.0, 0.0, 1.0;

    std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
    std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};

    std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);

    std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
    sframe users_query = sframe({users}, {"user"});


    auto ret_item_neighbor = model->get_similar_items(all_items, 4);

    auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

    std::vector<std::vector<flexible_type>> rows;
    ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                        rows);
    std::vector<std::vector<flexible_type>> truth;
    truth.reserve(ret_item_neighbor.size());

    // test get_similar_items
    for (size_t i = 0; i < 4; i++) {
      std::vector<std::pair<size_t, double> > item_pair_list;
      for (size_t j = 0; j < 4; j++) {
        if (i == j) continue;
        if (i < j)
          item_pair_list.push_back(std::make_pair(j, ans(i,j)));
        else
          item_pair_list.push_back(std::make_pair(j, ans(j,i)));
      }
      std::sort(item_pair_list.begin(), item_pair_list.end(),
                [](const std::pair<size_t, double>& a,
                   const std::pair<size_t, double>& b)
                {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
      size_t r = 0;
      for (size_t j = 0; j < item_pair_list.size(); j++) {
        if (item_pair_list[j].second == 0) continue;
        truth.push_back({all_items_vec[i],
                         all_items_vec[item_pair_list[j].first],
                         item_pair_list[j].second, ++r});
      }
    }

    for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){

      TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
      TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
      flexible_type score = rows[i][2];
      TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];
      DASSERT_EQ(item, truth[i][0]);
      DASSERT_EQ(neighbor, truth[i][1]);
      TS_ASSERT_DELTA(score, truth[i][2], DELTA);
      DASSERT_EQ(rank, truth[i][3]);

    }

    // test all items
    auto all_item_neighbors = model->get_similar_items(nullptr);
    DASSERT_EQ(all_item_neighbors.num_rows(), ret_item_neighbor.num_rows());

    {
      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d * (1. - a_mean)+ b_d *(.3 - b_mean) + c_d *(.5 - c_mean) )/(std::fabs(a_d) + std::fabs(b_d) + std::fabs(c_d)) + d_mean},
        {1, 0, (a_b * (.5 - b_mean)+ a_c *(0.6 - c_mean)) / (std::fabs(a_b) +std::fabs(a_c)) + a_mean},
        {1, 3, (b_d * (.5 - b_mean)+ c_d *(0.6 - c_mean)) / (std::fabs(b_d) + std::fabs(c_d)) + d_mean},
        {2, 0, (a_c * (1 - c_mean) + a_d * (1. - d_mean)) / (std::fabs(a_c) + std::fabs(a_d)) + a_mean},
        {2, 1, (b_c * (1. - c_mean) + b_d * (1 - d_mean)) / (std::fabs(b_c) + std::fabs(b_d)) + b_mean},
        {3,1, (a_b * (.1 - a_mean) + b_d * (1.5 - d_mean)) / (std::fabs(a_b) + std::fabs(b_d)) + b_mean},
        {3,2, (a_c * (.1 - a_mean)+ c_d * (1.5 - d_mean)) / (std::fabs(a_c) + std::fabs(c_d)) + c_mean}
      };

      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());

      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }
    }

    // Test cosine similarity
    model = std::make_shared<recsys::recsys_itemcf>();
    opts["similarity_type"] = "cosine";
    model->init_options(opts);
    model->setup_and_train(data);

    a_b = (1*.3) / std::sqrt(1+.1*.1) / std::sqrt(.3*.3 + .5*.5);
    a_c = (1*.5) / std::sqrt(1+.1*.1) / std::sqrt(.5*.5 + .6*.6 + 1*1);
    a_d = (.1*1.5) / std::sqrt(1+.1*.1) / std::sqrt(1+1.5*1.5);
    b_c = (.3*.5 + .5*.6) / std::sqrt(.3*.3 + .5*.5) / std::sqrt(.5*.5 + .6*.6 + 1.0);
    b_d = 0.0;
    c_d = 1.0 / std::sqrt(.5*.5 + .6*.6 + 1*1) / std::sqrt(1*1 + 1.5*1.5);

    ans << 1.,  a_b, a_c, a_d,
        0.0, 1.,  b_c, b_d,
        0.0, 0.0, 1.,  c_d,
        0.0, 0.0, 0.0, 1.;

    // test getting item neighbors
    {
      std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
      std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};

      std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);

      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
      sframe users_query = sframe({users}, {"user"});


      auto ret_item_neighbor = model->get_similar_items(all_items, 4);
      auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

      std::vector<std::vector<flexible_type>> rows;
      ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                          rows);

      std::vector<std::vector<flexible_type>> truth;
      truth.reserve(ret_item_neighbor.size());

      for (size_t i = 0; i < 4; i++) {
        std::vector<std::pair<size_t, double> > item_pair_list;
        for (size_t j = 0; j < 4; j++) {
          if (i == j) continue;
          if (i < j)
            item_pair_list.push_back(std::make_pair(j, ans(i,j)));
          else
            item_pair_list.push_back(std::make_pair(j, ans(j,i)));
        }
        std::sort(item_pair_list.begin(), item_pair_list.end(),
                  [](const std::pair<size_t, double>& a,
                     const std::pair<size_t, double>& b)
                  {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
        size_t r = 0;
        for (size_t j = 0; j < item_pair_list.size(); j++) {
          if (item_pair_list[j].second == 0) continue;
          truth.push_back({all_items_vec[i],
                           all_items_vec[item_pair_list[j].first],
                           item_pair_list[j].second, ++r});
        }
      }

      for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){

        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
        flexible_type score = rows[i][2];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];

        DASSERT_EQ(item, truth[i][0]);
        DASSERT_EQ(neighbor, truth[i][1]);
        TS_ASSERT_DELTA(score, truth[i][2], DELTA);
        DASSERT_EQ(rank, truth[i][3]);
      }

      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d * 1. + b_d * .3 + c_d * .5 )/(std::fabs(a_d) + std::fabs(b_d) + std::fabs(c_d)) },
        {1, 0, (a_b * .5 + a_c * 0.6) / (std::fabs(a_b) +std::fabs(a_c)) },
        {1, 3, (b_d * .5 + c_d * 0.6) / (std::fabs(b_d) + std::fabs(c_d))},
        {2, 0, (a_c * 1 + a_d * 1.) / (std::fabs(a_c) + std::fabs(a_d))},
        {2, 1, (b_c * 1. + b_d * 1) / (std::fabs(b_c) + std::fabs(b_d))},
        {3,1, (a_b * .1 + b_d * 1.5) / (std::fabs(a_b) + std::fabs(b_d))},
        {3,2, (a_c * .1+ c_d * 1.5) / (std::fabs(a_c) + std::fabs(c_d))}
      };

      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());

      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }


    }

    /////////////////////////////
    // Test jaccard similarity
    model = std::make_shared<recsys::recsys_itemcf>();
    opts["similarity_type"] = "jaccard";
    model->init_options(opts);
    model->setup_and_train(data);

    a_b = 1./3.;
    a_c = 1./4.;
    a_d = 1./3.;
    b_c = 2./3.;
    b_d = 0.;
    c_d = 1./4.;

    // removed the diagonal values
    ans << 1.,  a_b, a_c, a_d,
        0.0, 1.,  b_c, b_d,
        0.0, 0.0, 1.,  c_d,
        0.0, 0.0, 0.0, 1.;

    // test getting item neighbors
    {
      std::vector<flexible_type> all_users_vec = {"0", "1", "2", "3"};
      std::vector<flexible_type> all_items_vec = {"A", "B", "C", "D"};

      std::shared_ptr<sarray<flexible_type>> all_items = make_testing_sarray(
          flex_type_enum::STRING, all_items_vec);
      std::shared_ptr<sarray<flexible_type>> users = make_testing_sarray(
          flex_type_enum::STRING, all_users_vec);
      sframe users_query = sframe({users}, {"user"});


      auto ret_item_neighbor = model->get_similar_items(all_items, 4);

      auto ret_item_neighbor_reader = ret_item_neighbor.get_reader();

      std::vector<std::vector<flexible_type>> rows;
      ret_item_neighbor_reader->read_rows(0, ret_item_neighbor.num_rows(),
                                          rows);

      std::vector<std::vector<flexible_type>> truth;
      truth.reserve(ret_item_neighbor.size());

      for (size_t i = 0; i < 4; i++) {
        std::vector<std::pair<size_t, double> > item_pair_list;
        for (size_t j = 0; j < 4; j++) {
          if (i == j) continue;
          if (i < j)
            item_pair_list.push_back(std::make_pair(j, ans(i,j)));
          else
            item_pair_list.push_back(std::make_pair(j, ans(j,i)));
        }
        std::sort(item_pair_list.begin(), item_pair_list.end(),
                  [](const std::pair<size_t, double>& a,
                     const std::pair<size_t, double>& b)
                  {return (a.second != b.second) ? a.second > b.second : a.first < b.first;} );
        size_t r = 0;
        for (size_t j = 0; j < item_pair_list.size(); j++) {
          if (item_pair_list[j].second == 0) continue;
          truth.push_back({all_items_vec[i],
                           all_items_vec[item_pair_list[j].first],
                           item_pair_list[j].second, ++r});
        }
      }

      for (size_t i = 0; i < ret_item_neighbor.num_rows(); i++){

        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t item = rows[i][0];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t neighbor = rows[i][1];
        flexible_type score = rows[i][2];
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t rank = rows[i][3];
        DASSERT_EQ(item, truth[i][0]);
        DASSERT_EQ(neighbor, truth[i][1]);

        TS_ASSERT_DELTA(score, truth[i][2], DELTA);
        DASSERT_EQ(rank, truth[i][3]);
      }

      // test recommend
      //
      std::vector<std::vector<flexible_type>> rec_truth = {
        {0, 3, (a_d * 1. + b_d * .3 + c_d * .5 )/(std::fabs(a_d) + std::fabs(b_d) + std::fabs(c_d)) },
        {1, 0, (a_b * .5 + a_c * 0.6) / (std::fabs(a_b) +std::fabs(a_c)) },
        {1, 3, (b_d * .5 + c_d * 0.6) / (std::fabs(b_d) + std::fabs(c_d))},
        {2, 0, (a_c * 1 + a_d * 1.) / (std::fabs(a_c) + std::fabs(a_d))},
        {2, 1, (b_c * 1. + b_d * 1) / (std::fabs(b_c) + std::fabs(b_d))},
        {3,1, (a_b * .1 + b_d * 1.5) / (std::fabs(a_b) + std::fabs(b_d))},
        {3,2, (a_c * .1+ c_d * 1.5) / (std::fabs(a_c) + std::fabs(c_d))}
      };

      auto recs = model->recommend(users_query, 4);
      std::vector<std::vector<flexible_type>> rec_rows;
      recs.get_reader()->read_rows(0, recs.size(), rec_rows);
      DASSERT_EQ(rec_truth.size(), rec_rows.size());

      std::sort(rec_rows.begin(), rec_rows.end(),
                [](const std::vector<flexible_type>& a,
                   const std::vector<flexible_type>& b){
                return a[0] != b[0] ? a[0] < b[0] : a[1] < b[1];
                });

      for (size_t i = 0; i < rec_truth.size(); i++) {
        TS_ASSERT_EQUALS(all_users_vec[rec_truth[i][0]], rec_rows[i][0]);
        TS_ASSERT_EQUALS(all_items_vec[rec_truth[i][1]], rec_rows[i][1]);
        TS_ASSERT_DELTA(rec_truth[i][2], rec_rows[i][2].get<flex_float>(), DELTA);
      }

      {
        auto num_items_per_user = model->get_num_items_per_user();
        std::vector<std::vector<flexible_type>> result_rows;
        num_items_per_user.get_reader()->read_rows(0, num_items_per_user.size(), result_rows);
        std::vector<std::string> true_users = {"0", "1", "2", "3"};
        std::vector<size_t> true_counts = {3, 2, 2, 2};
        for (size_t i = 0; i < true_users.size(); ++i) {
          TS_ASSERT_EQUALS(true_users[i], result_rows[i][0]);
          TS_ASSERT_EQUALS(true_counts[i], result_rows[i][1]);
        }
      }
    }
  }

  void _test_similarity_computation() {

    // test similarity computation and recommendations
    similarity_computation_without_rating("auto");

    // test similarity computation and recommendations
    similarity_computation_with_rating("auto");
  }

};

BOOST_FIXTURE_TEST_SUITE(_recsys_itemcf_test, recsys_itemcf_test)
BOOST_AUTO_TEST_CASE(test_itemcf_jaccard) {
  recsys_itemcf_test::test_itemcf_jaccard();
}
BOOST_AUTO_TEST_CASE(test_itemcf_cosine) {
  recsys_itemcf_test::test_itemcf_cosine();
}
BOOST_AUTO_TEST_CASE(test_itemcf_jaccard_2) {
  recsys_itemcf_test::test_itemcf_jaccard_2();
}
BOOST_AUTO_TEST_CASE(test_itemcf_pearson_2) {
  recsys_itemcf_test::test_itemcf_pearson_2();
}
BOOST_AUTO_TEST_SUITE_END()
