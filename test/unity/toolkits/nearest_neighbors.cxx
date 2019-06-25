#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>
#include <core/random/random.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>

// Nearest neighbors
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <toolkits/nearest_neighbors/lsh_neighbors.hpp>


using namespace turi;


struct test_nearest_neighbors_utils  {

  public:

  void test_upper_triangle_indices() {
    std::cout << std::endl;

    // Typical usage
    std::pair<size_t, size_t> matrix_indices;

    matrix_indices = nearest_neighbors::upper_triangular_indices(7, 5);
    TS_ASSERT_EQUALS(matrix_indices.first, 1);
    TS_ASSERT_EQUALS(matrix_indices.second, 3);

    matrix_indices = nearest_neighbors::upper_triangular_indices(0, 5);
    TS_ASSERT_EQUALS(matrix_indices.first, 0);
    TS_ASSERT_EQUALS(matrix_indices.second, 0);

    matrix_indices = nearest_neighbors::upper_triangular_indices(14, 5);
    TS_ASSERT_EQUALS(matrix_indices.first, 4);
    TS_ASSERT_EQUALS(matrix_indices.second, 4);


#if DEBUG
    // Index out of bounds errors
    TS_ASSERT_THROWS_ANYTHING(nearest_neighbors::upper_triangular_indices(0, 0));
    TS_ASSERT_THROWS_ANYTHING(nearest_neighbors::upper_triangular_indices(100, 5));
#endif
  }

  void test_distance_name_extraction() {

    auto distance_fn = function_closure_info();
    distance_fn.native_fn_name = "_distances.fossa_distance";
    std::string dist_name = nearest_neighbors::extract_distance_function_name(distance_fn);
    
    std::string ans = "fossa_distance";
    ASSERT_EQ(dist_name, ans);
  }

  void test_block_number_calculation() {

    // Arguments to 'calculate_num_blocks':
    // 1. num_ref_examples
    // 2. num_query_examples
    // 3. dimension
    // 4. max_thread_memory
    // 5. min_ref_blocks
    // 6. min_query_blocks
    //
    // 'calculate_num_blocks' returns a pair of size_t values: ref blocks and
    // query blocks.
    
    size_t max_thread_mem = 1024 * 1024 * 1024; // 1GB

    // Small data, 1 query, no min blocks
    std::pair<size_t, size_t> num_blocks = nearest_neighbors::calculate_num_blocks(731, 1, 1000, max_thread_mem, 1, 1);
    TS_ASSERT_EQUALS(num_blocks.first , 1);
    TS_ASSERT_EQUALS(num_blocks.second , 1);

    // Small data, 1 query, min ref blocks
    num_blocks = nearest_neighbors::calculate_num_blocks(731, 1, 1000, max_thread_mem, 4, 1);
    TS_ASSERT_EQUALS(num_blocks.first, 4);
    TS_ASSERT_EQUALS(num_blocks.second , 1);

    // More blocks than min ref blocks
    num_blocks = nearest_neighbors::calculate_num_blocks(10000, 1, 5, 1024 * 128, 8, 1);
    TS_ASSERT_EQUALS(num_blocks.first, 82);
  }

  void test_all_pairs_squared_euclidean() {
    nearest_neighbors::DenseMatrix A(4, 2);
    nearest_neighbors::DenseMatrix B(3, 2);
    
    A << 1, 1,
         4, 4,
         5, 5,
         2, 2;

    B << 1, 2,
         4, 4,
         3, 5;

    nearest_neighbors::DenseMatrix dists(4, 3);
    nearest_neighbors::all_pairs_squared_euclidean(A, B, dists);

    nearest_neighbors::DenseMatrix ans(4, 3);
    ans << 1, 18, 20,
           13, 0, 2,
           25, 2, 4,
           1, 8, 10;

    ASSERT_EQ(dists, ans);
  }
};


struct test_similarity_graph  {

public:

  void run_sim_graph_test(const std::string& model,
                          const std::string& run_string,
                          const std::string& distance) {

    global_logger().set_log_level(LOG_ERROR);
    random::seed(0);

    // Create random data.
    size_t n = 5;
    sframe data = make_random_sframe(n, run_string, false);
    std::vector<std::vector<flexible_type>> label_vec(n);

    for (size_t i = 0; i < n; ++i) {
      label_vec[i] = {std::to_string(hash64(i))};
    }
    sframe labels = make_testing_sframe({"label"}, label_vec);

    // Initialize the model, composite distance, and model options.
    auto fn = function_closure_info();
    fn.native_fn_name = "_distances.";
    fn.native_fn_name += distance;
    nearest_neighbors::dist_component_type p = std::make_tuple(data.column_names(), fn, 1.0);
    std::vector<nearest_neighbors::dist_component_type> composite_params = {p};

    std::map<std::string, flexible_type> nn_options;
 
    if (model == "lsh") {
      nn_options["num_tables"] = 4;
      nn_options["num_projections_per_table"] = 4;
    }

    std::shared_ptr<nearest_neighbors::nearest_neighbors_model> m;

    if (model == "brute_force") {
      m.reset(new nearest_neighbors::brute_force_neighbors);
    } else if (model == "ball_tree") {
      m.reset(new nearest_neighbors::ball_tree_neighbors);
    } else if (model == "lsh") {
      m.reset(new nearest_neighbors::lsh_neighbors);
    }

    // Train the model, compute the similarity graph, and check both
    // plausibility and equality with query results.
    m->train(data, labels, composite_params, nn_options);

    size_t k = 2;
    
    // include_self_edges = fales
    sframe sim_graph = m->similarity_graph(k, -1, false);
    sframe knn = m->query(data, labels, k + 1, -1);

    TS_ASSERT_EQUALS(sim_graph.num_columns(), 4);
    if (model != "lsh") {
      TS_ASSERT_EQUALS(sim_graph.num_rows(), 10);
    }

    auto sim_graph_vec = testing_extract_sframe_data(sim_graph);
    auto temp_knn_vec = testing_extract_sframe_data(knn);

    // Remove the self-edges from the query output.
    std::vector<std::vector<flexible_type> > knn_vec;

    for (size_t i = 0; i < temp_knn_vec.size(); ++i) {
      if (temp_knn_vec[i][3] != 1) {
        knn_vec.push_back(temp_knn_vec[i]);
      }
    }

    std::sort(knn_vec.begin(), knn_vec.end(),
              [&](const std::vector<flexible_type>& a, const std::vector<flexible_type>& b) {
                return std::tie(a[0], a[2], a[3]) < std::tie(b[0], b[2], b[3]);
              });
    std::sort(sim_graph_vec.begin(), sim_graph_vec.end(),
              [&](const std::vector<flexible_type>& a, const std::vector<flexible_type>& b) {
                return std::tie(a[0], a[2], a[3]) < std::tie(b[0], b[2], b[3]);
              });
    TS_ASSERT_EQUALS(knn_vec.size(), sim_graph_vec.size());

    // Check for equality
    for (size_t i = 0; i < sim_graph_vec.size(); ++i) {
      TS_ASSERT_EQUALS(sim_graph_vec[i][0], knn_vec[i][0]);       // query label
      // TS_ASSERT_EQUALS(sim_graph_vec[i][1], knn_vec[i][1]);       // ref label not always equal because of ties
      TS_ASSERT_DELTA(sim_graph_vec[i][2], knn_vec[i][2], 1e-8);  // distance
      TS_ASSERT_EQUALS(sim_graph_vec[i][3], knn_vec[i][3] - 1);   // rank
    }
  }

  // Test various distances.
  void test_brute_force_dist1() {
    run_sim_graph_test("brute_force", "nnn", "euclidean");
  }

  void test_brute_force_dist2() {
    run_sim_graph_test("brute_force", "nnn", "squared_euclidean");
  }

  void test_brute_force_dist3() {
    run_sim_graph_test("brute_force", "nnn", "manhattan");
  }

  void test_brute_force_dist4() {
    run_sim_graph_test("brute_force", "nnn", "cosine");
  }

  // Test various data types.
  void test_brute_force_data1() {
    run_sim_graph_test("brute_force", "V", "euclidean");  // 1000 numeric features
  }

  void test_brute_force_data2() {
    run_sim_graph_test("brute_force", "z", "euclidean");  // categorical
  }

  void test_brute_force_data3() {
    run_sim_graph_test("brute_force", "d", "euclidean");  // dictionary
  }

  void test_ball_tree_dist1() {
    run_sim_graph_test("ball_tree", "nnn", "euclidean");
  }

  void test_ball_tree_dist2() {
    run_sim_graph_test("ball_tree", "nnn", "squared_euclidean");
  }

  void test_ball_tree_dist3() {
    run_sim_graph_test("ball_tree", "nnn", "manhattan");
  }

  void test_ball_tree_dist4() {
    run_sim_graph_test("ball_tree", "nnn", "cosine");
  }

  // Test various data types.
  void test_ball_tree_data1() {
    run_sim_graph_test("ball_tree", "V", "euclidean");  // 1000 numeric features
  }

  void test_ball_tree_data2() {
    run_sim_graph_test("ball_tree", "z", "euclidean");  // categorical
  }

  void test_ball_tree_data3() {
    run_sim_graph_test("ball_tree", "d", "euclidean");  // dictionary
  }

  void test_lsh_dist1() {
    run_sim_graph_test("lsh", "nnn", "euclidean");
  }

  void test_lsh_dist2() {
    run_sim_graph_test("lsh", "nnn", "squared_euclidean");
  }

  void test_lsh_dist3() {
    run_sim_graph_test("lsh", "nnn", "manhattan");
  }

  void test_lsh_dist4() {
    run_sim_graph_test("lsh", "nnn", "cosine");
  }


  // Test various data types.
  void test_lsh_data1() {
    run_sim_graph_test("lsh", "V", "euclidean");  // 1000 numeric features
  }

  void test_lsh_data2() {
    run_sim_graph_test("lsh", "z", "euclidean");  // categorical
  }

  void test_lsh_data3() {
    run_sim_graph_test("lsh", "d", "euclidean");  // dictionary
  }
};


struct test_nn_consistency  {

 public:

  void run_nn_test(const std::string& model,
                   size_t n,
                   const std::string& run_string,
                   const std::string& distance) {
    
    global_logger().set_log_level(LOG_ERROR);

    random::seed(0);

    sframe data[] = {make_random_sframe(n, run_string, false),
                     make_random_sframe(5, run_string, false),
                     make_random_sframe(2, run_string, false)};

    sframe y[3];

    for(size_t i = 0; i < 3; ++i) {
      std::vector<std::vector<flexible_type> > labels(data[i].size());

      for(size_t j = 0; j < data[i].size(); ++j) {
        labels[j] = {std::to_string(hash64(i, j))};
      }
      y[i] = make_testing_sframe({"label"}, labels);
    }

    std::shared_ptr<nearest_neighbors::nearest_neighbors_model> nn, nn_sl_1, nn_sl_2;

    // Construct the model
    if(model == "ball_tree") {
      nn.reset(new nearest_neighbors::ball_tree_neighbors);
      nn_sl_1.reset(new nearest_neighbors::ball_tree_neighbors);
      nn_sl_2.reset(new nearest_neighbors::ball_tree_neighbors);
    } else if (model == "brute_force") {
      nn.reset(new nearest_neighbors::brute_force_neighbors);
      nn_sl_1.reset(new nearest_neighbors::brute_force_neighbors);
      nn_sl_2.reset(new nearest_neighbors::brute_force_neighbors);
    } else if (model == "lsh") {
      nn.reset(new nearest_neighbors::lsh_neighbors);
      nn_sl_1.reset(new nearest_neighbors::lsh_neighbors);
      nn_sl_2.reset(new nearest_neighbors::lsh_neighbors);
    }

    // Temp: Need to construct a set of composite params
    auto fn = function_closure_info();
    fn.native_fn_name = "_distances.";
    fn.native_fn_name += distance;
    nearest_neighbors::dist_component_type p = std::make_tuple(data[0].column_names(), fn, 1.0);
    std::vector<nearest_neighbors::dist_component_type> composite_params = {p};

    std::map<std::string, flexible_type> nn_options;
    
    if (model == "lsh") {
      nn_options["num_tables"] = 4;
      nn_options["num_projections_per_table"] = 4;
    }

    nn->train(data[0], y[0], composite_params, nn_options);
    
    save_and_load_object(*nn_sl_1, *nn);

    size_t q_idx_v[] = {1, 2};
    size_t k_v[] = {1, 2, nearest_neighbors::NONE_FLAG};
    double radius_v[] = {0.1, 1, 5};

    parallel_for(size_t(0), size_t(2 * 3 * 3), [&](size_t main_idx) {

        size_t q_idx  = q_idx_v[main_idx / (3 * 3)];
        size_t k      = k_v[(main_idx / 3) % 3];
        double radius = radius_v[main_idx % 3];
        
        std::vector<std::vector<flexible_type> > result_1
            = testing_extract_sframe_data(nn->query(data[q_idx], y[q_idx], k, radius));

        std::vector<std::vector<flexible_type> > result_2
            = testing_extract_sframe_data(nn_sl_1->query(data[q_idx], y[q_idx], k, radius));

        ASSERT_TRUE(result_1 == result_2);
      });

    save_and_load_object(*nn_sl_2, *nn);

    parallel_for(size_t(0), size_t(2 * 3 * 3), [&](size_t main_idx) {

        size_t q_idx = q_idx_v[main_idx / (3 * 3)];
        size_t k = (main_idx / 3) % 3;
        double radius = main_idx % 3;

        std::vector<std::vector<flexible_type> > result_1
            = testing_extract_sframe_data(nn->query(data[q_idx], y[q_idx], k, radius));

        std::vector<std::vector<flexible_type> > result_2
            = testing_extract_sframe_data(nn_sl_2->query(data[q_idx], y[q_idx], k, radius));

        ASSERT_TRUE(result_1 == result_2);
      });
  }

  void test_ball_tree_n_1() {
    run_nn_test("ball_tree", 30, "n", "euclidean");
  }

  void test_ball_tree_n_2() {
    run_nn_test("ball_tree", 30, "n", "manhattan");
  }

  void test_ball_tree_nnnnnn_1() {
    run_nn_test("ball_tree", 30, "nnnnnn", "euclidean");
  }

  void test_ball_tree_nnnnnn_2() {
    run_nn_test("ball_tree", 30, "nnnnnn", "manhattan");
  }

  void test_ball_tree_nd_1() {
    run_nn_test("ball_tree", 30, "nd", "euclidean");
  }

  void test_ball_tree_nd_2() {
    run_nn_test("ball_tree", 30, "nd", "manhattan");
  }

  void test_lsh_euclidean_1() {
    run_nn_test("lsh", 100, "V", "euclidean");
  }

  void test_lsh_euclidean_2() {
    run_nn_test("lsh", 100, "d", "euclidean");
  }
   
  void test_lsh_squared_euclidean_1() {
    run_nn_test("lsh", 100, "V", "squared_euclidean");
  }

  void test_lsh_squared_euclidean_2() {
    run_nn_test("lsh", 100, "d", "squared_euclidean");
  }
 
  void test_lsh_manhattan_1() {
    run_nn_test("lsh", 100, "V", "manhattan");
  }

  void test_lsh_manhattan_2() {
    run_nn_test("lsh", 100, "d", "manhattan");
  }

  void test_lsh_cosine_1() {
    run_nn_test("lsh", 100, "V", "cosine");
  }

  void test_lsh_cosine_2() {
    run_nn_test("lsh", 100, "d", "cosine");
  }

  void test_lsh_jaccard_1() {
    run_nn_test("lsh", 100, "D", "jaccard");
  }

  void test_lsh_jaccard_2() {
    run_nn_test("lsh", 1000, "D", "jaccard");
  }

  void test_lsh_dot_product_1() {
    run_nn_test("lsh", 100, "D", "dot_product");
  }

  void test_lsh_dot_product_2() {
    run_nn_test("lsh", 100, "V", "dot_product");
  }

  void test_lsh_transformed_dot_product_1() {
    run_nn_test("lsh", 100, "D", "transformed_dot_product");
  }

  void test_lsh_transformed_dot_product_2() {
    run_nn_test("lsh", 100, "V", "transformed_dot_product");
  }

  void test_ball_tree_n_1_large() {
    run_nn_test("ball_tree", 100, "n", "euclidean");
  }

  void test_ball_tree_n_2_large() {
    run_nn_test("ball_tree", 100, "n", "manhattan");
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_brute_force_n_1() {
    run_nn_test("brute_force", 30, "n", "euclidean");
  }

  void test_brute_force_n_2() {
    run_nn_test("brute_force", 30, "n", "manhattan");
  }

  // void test_brute_force_n_3() {
  //   run_nn_test("brute_force", 30, "n", "jaccard");
  // }
  //
  // void test_brute_force_n_4() {
  //   run_nn_test("brute_force", 30, "n", "weighted_jaccard");
  // }
  //
  void test_brute_force_n_5() {
    run_nn_test("brute_force", 30, "n", "cosine");
  }

  // void test_brute_force_n_6() {
  //   run_nn_test("brute_force", 30, "n", "auto");
  // }

  ////////////////////////////////////////////////////////////////////////////////

  void test_brute_force_d_1() {
    run_nn_test("brute_force", 30, "nd", "euclidean");
  }

  void test_brute_force_d_2() {
    run_nn_test("brute_force", 30, "nd", "manhattan");
  }

  // void test_brute_force_d_3() {
  //   run_nn_test("brute_force", 30, "nd", "jaccard");
  // }
  //
  // void test_brute_force_d_4() {
  //   run_nn_test("brute_force", 30, "nd", "weighted_jaccard");
  // }

  void test_brute_force_d_5() {
    run_nn_test("brute_force", 30, "nd", "cosine");
  }

  // void test_brute_force_d_6() {
  //   run_nn_test("brute_force", 30, "nd", "auto");
  // }

  ////////////////////////////////////////////////////////////////////////////////

  void test_brute_force_nnnnnn_1() {
    run_nn_test("brute_force", 30, "nnnnnn", "euclidean");
  }

  void test_brute_force_nnnnnn_2() {
    run_nn_test("brute_force", 30, "nnnnnn", "manhattan");
  }

  // void test_brute_force_nnnnnn_3() {
  //   run_nn_test("brute_force", 30, "nnnnnn", "jaccard");
  // }
  //
  // void test_brute_force_nnnnnn_4() {
  //   run_nn_test("brute_force", 30, "nnnnnn", "weighted_jaccard");
  // }
  //
  void test_brute_force_nnnnnn_5() {
    run_nn_test("brute_force", 30, "nnnnnn", "cosine");
  }

  // void test_brute_force_nnnnnn_6() {
  //   run_nn_test("brute_force", 30, "nnnnnn", "auto");
  // }

  ////////////////////////////////////////////////////////////////////////////////

  // void test_brute_force_ndvus_1() {
  //   run_nn_test("brute_force", 30, "ndvus", "euclidean");
  // }
  //
  // void test_brute_force_ndvus_2() {
  //   run_nn_test("brute_force", 30, "ndvus", "manhattan");
  // }
  //
  // void test_brute_force_ndvus_3() {
  //   run_nn_test("brute_force", 30, "ndvus", "jaccard");
  // }
  //
  // void test_brute_force_ndvus_4() {
  //   run_nn_test("brute_force", 30, "ndvus", "weighted_jaccard");
  // }
  //
  // void test_brute_force_ndvus_5() {
  //   run_nn_test("brute_force", 30, "ndvus", "cosine");
  // }
  //
  // void test_brute_force_ndvus_6() {
  //   run_nn_test("brute_force", 30, "ndvus", "auto");
  // }

  ////////////////////////////////////////////////////////////////////////////////

  void test_brute_force_n_1_large() {
    run_nn_test("brute_force", 100, "n", "euclidean");
  }

  void test_brute_force_n_2_large() {
    run_nn_test("brute_force", 100, "n", "manhattan");
  }

  // void test_brute_force_n_3_large() {
  //   run_nn_test("brute_force", 100, "n", "jaccard");
  // }
  //
  // void test_brute_force_n_4_large() {
  //   run_nn_test("brute_force", 100, "n", "weighted_jaccard");
  // }
  //
  void test_brute_force_n_5_large() {
    run_nn_test("brute_force", 100, "n", "cosine");
  }

  // void test_brute_force_n_6_large() {
  //   run_nn_test("brute_force", 100, "n", "auto");
  // }


};

BOOST_FIXTURE_TEST_SUITE(_test_nearest_neighbors_utils, test_nearest_neighbors_utils)
BOOST_AUTO_TEST_CASE(test_upper_triangle_indices) {
  test_nearest_neighbors_utils::test_upper_triangle_indices();
}
BOOST_AUTO_TEST_CASE(test_distance_name_extraction) {
  test_nearest_neighbors_utils::test_distance_name_extraction();
}
BOOST_AUTO_TEST_CASE(test_block_number_calculation) {
  test_nearest_neighbors_utils::test_block_number_calculation();
}
BOOST_AUTO_TEST_CASE(test_all_pairs_squared_euclidean) {
  test_nearest_neighbors_utils::test_all_pairs_squared_euclidean();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_test_similarity_graph, test_similarity_graph)
BOOST_AUTO_TEST_CASE(test_brute_force_dist1) {
  test_similarity_graph::test_brute_force_dist1();
}
BOOST_AUTO_TEST_CASE(test_brute_force_dist2) {
  test_similarity_graph::test_brute_force_dist2();
}
BOOST_AUTO_TEST_CASE(test_brute_force_dist3) {
  test_similarity_graph::test_brute_force_dist3();
}
BOOST_AUTO_TEST_CASE(test_brute_force_dist4) {
  test_similarity_graph::test_brute_force_dist4();
}
BOOST_AUTO_TEST_CASE(test_brute_force_data1) {
  test_similarity_graph::test_brute_force_data1();
}
BOOST_AUTO_TEST_CASE(test_brute_force_data2) {
  test_similarity_graph::test_brute_force_data2();
}
BOOST_AUTO_TEST_CASE(test_brute_force_data3) {
  test_similarity_graph::test_brute_force_data3();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_dist1) {
  test_similarity_graph::test_ball_tree_dist1();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_dist2) {
  test_similarity_graph::test_ball_tree_dist2();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_dist3) {
  test_similarity_graph::test_ball_tree_dist3();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_dist4) {
  test_similarity_graph::test_ball_tree_dist4();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_data1) {
  test_similarity_graph::test_ball_tree_data1();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_data2) {
  test_similarity_graph::test_ball_tree_data2();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_data3) {
  test_similarity_graph::test_ball_tree_data3();
}
BOOST_AUTO_TEST_CASE(test_lsh_dist1) {
  test_similarity_graph::test_lsh_dist1();
}
BOOST_AUTO_TEST_CASE(test_lsh_dist2) {
  test_similarity_graph::test_lsh_dist2();
}
BOOST_AUTO_TEST_CASE(test_lsh_dist3) {
  test_similarity_graph::test_lsh_dist3();
}
BOOST_AUTO_TEST_CASE(test_lsh_dist4) {
  test_similarity_graph::test_lsh_dist4();
}
BOOST_AUTO_TEST_CASE(test_lsh_data1) {
  test_similarity_graph::test_lsh_data1();
}
BOOST_AUTO_TEST_CASE(test_lsh_data2) {
  test_similarity_graph::test_lsh_data2();
}
BOOST_AUTO_TEST_CASE(test_lsh_data3) {
  test_similarity_graph::test_lsh_data3();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_test_nn_consistency, test_nn_consistency)
BOOST_AUTO_TEST_CASE(test_ball_tree_n_1) {
  test_nn_consistency::test_ball_tree_n_1();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_n_2) {
  test_nn_consistency::test_ball_tree_n_2();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_nnnnnn_1) {
  test_nn_consistency::test_ball_tree_nnnnnn_1();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_nnnnnn_2) {
  test_nn_consistency::test_ball_tree_nnnnnn_2();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_nd_1) {
  test_nn_consistency::test_ball_tree_nd_1();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_nd_2) {
  test_nn_consistency::test_ball_tree_nd_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_euclidean_1) {
  test_nn_consistency::test_lsh_euclidean_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_euclidean_2) {
  test_nn_consistency::test_lsh_euclidean_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_squared_euclidean_1) {
  test_nn_consistency::test_lsh_squared_euclidean_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_squared_euclidean_2) {
  test_nn_consistency::test_lsh_squared_euclidean_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_manhattan_1) {
  test_nn_consistency::test_lsh_manhattan_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_manhattan_2) {
  test_nn_consistency::test_lsh_manhattan_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_cosine_1) {
  test_nn_consistency::test_lsh_cosine_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_cosine_2) {
  test_nn_consistency::test_lsh_cosine_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_jaccard_1) {
  test_nn_consistency::test_lsh_jaccard_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_jaccard_2) {
  test_nn_consistency::test_lsh_jaccard_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_dot_product_1) {
  test_nn_consistency::test_lsh_dot_product_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_dot_product_2) {
  test_nn_consistency::test_lsh_dot_product_2();
}
BOOST_AUTO_TEST_CASE(test_lsh_transformed_dot_product_1) {
  test_nn_consistency::test_lsh_transformed_dot_product_1();
}
BOOST_AUTO_TEST_CASE(test_lsh_transformed_dot_product_2) {
  test_nn_consistency::test_lsh_transformed_dot_product_2();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_n_1_large) {
  test_nn_consistency::test_ball_tree_n_1_large();
}
BOOST_AUTO_TEST_CASE(test_ball_tree_n_2_large) {
  test_nn_consistency::test_ball_tree_n_2_large();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_1) {
  test_nn_consistency::test_brute_force_n_1();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_2) {
  test_nn_consistency::test_brute_force_n_2();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_5) {
  test_nn_consistency::test_brute_force_n_5();
}
BOOST_AUTO_TEST_CASE(test_brute_force_d_1) {
  test_nn_consistency::test_brute_force_d_1();
}
BOOST_AUTO_TEST_CASE(test_brute_force_d_2) {
  test_nn_consistency::test_brute_force_d_2();
}
BOOST_AUTO_TEST_CASE(test_brute_force_d_5) {
  test_nn_consistency::test_brute_force_d_5();
}
BOOST_AUTO_TEST_CASE(test_brute_force_nnnnnn_1) {
  test_nn_consistency::test_brute_force_nnnnnn_1();
}
BOOST_AUTO_TEST_CASE(test_brute_force_nnnnnn_2) {
  test_nn_consistency::test_brute_force_nnnnnn_2();
}
BOOST_AUTO_TEST_CASE(test_brute_force_nnnnnn_5) {
  test_nn_consistency::test_brute_force_nnnnnn_5();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_1_large) {
  test_nn_consistency::test_brute_force_n_1_large();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_2_large) {
  test_nn_consistency::test_brute_force_n_2_large();
}
BOOST_AUTO_TEST_CASE(test_brute_force_n_5_large) {
  test_nn_consistency::test_brute_force_n_5_large();
}
BOOST_AUTO_TEST_SUITE_END()
