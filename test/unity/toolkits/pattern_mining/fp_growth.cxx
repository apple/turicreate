#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/pattern_mining/fp_growth.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::pattern_mining;

/**
 *  Run tests.
*/
struct fp_growth_test  {

  public:
    // tests closet_algorithm
    void testCLOSETAlgorithm1(void){
      gl_sarray database = gl_sarray({flex_list{0,1,4,6},
                                      flex_list{1,2,4},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 3;

      fp_results_tree closed_itemset_tree = closet_algorithm(database, min_support);
      // Tree should be
      // (root (6:3, 4:6 (1:4, 2:4), 2:5))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {4}:6, {2}:5, {1,4}:4, {2,4}:4, {6}:3,
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 5);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes[0]->item_id, 6);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes[0]->item_count, 3);
    }

    void testCLOSETAlgorithm2(void){
      gl_sarray database = gl_sarray({flex_list{1},
                                      flex_list{1,2},
                                      flex_list{1,2,3},
                                      flex_list{1,2,3,4},
                                      flex_list{1,2,3,4,5}});
      size_t min_support = 1;

      fp_results_tree closed_itemset_tree = closet_algorithm(database, min_support);
      // Tree should be
      // (root (1:5 (2:4 (3:3 (4:2 (5:1))))))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {1}:5, {1,2}:4, {1,2,3}:3, {1,2,3,4}:2, {1,2,3,4,5}:1
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 5);
      auto root_node = closed_itemset_tree.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 5);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 4);

    }

    void testCLOSETAlgorithm3(void){
      gl_sarray database = gl_sarray({flex_list{1},
                                      flex_list{1,2},
                                      flex_list{1,2,3},
                                      flex_list{1,2,4}});
      size_t min_support = 1;

      fp_results_tree closed_itemset_tree = closet_algorithm(database, min_support);
      // Tree should be
      // (root (1:4 (2:3 (3:1, 4:1))))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {1}:4, {1,2}:3, {1,2,3}:1, {1,2,4}:1
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 4);
      auto root_node = closed_itemset_tree.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 3);

    }
    void testCLOSETAlgorithm4(void){
      gl_sarray database = gl_sarray({flex_list{1},
                                      flex_list{1,2},
                                      flex_list{1,2,3},
                                      flex_list{1,2,4},
                                      flex_list{5},
                                      flex_list{5}});
      size_t min_support = 1;

      fp_results_tree closed_itemset_tree = closet_algorithm(database, min_support);
      // Tree should be
      // (root (1:4 (2:3 (3:1, 4:1)), 5:2))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {1}:4, {1,2}:3, {1,2,3}:1, {1,2,4}:1, {5}:2
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 5);
      auto root_node = closed_itemset_tree.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 3);

    }
    void testCLOSETAlgorithm5(void){
      gl_sarray database = gl_sarray({flex_list{0,1,4,6},
                                      flex_list{1,2,4},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 1;

      fp_results_tree closed_itemset_tree = closet_algorithm(database, min_support);
      // Tree should be
      // header: {4:6, 2:5, 1:4, 6:3, 0:2, 3:2, 5:1}
      // (root (4:6 (2:4(5:1, 1:2 (3:1), 6:1), 1:4 (3:2, 6:1 (0:1)), 6:2),
      //        2:5 (6:2 (0:1)), 6:3 (0:2)))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should have (15 itemsest)
      //std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 15);
    }

    // Test top_k_algorithm
    void testTOPKAlgorithm1(void){
      // Filter on top_k
      gl_sarray database = gl_sarray({flex_list{0,1,4,6},
                                      flex_list{1,2,4},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 1;
      size_t top_k = 5;
      size_t min_length = 1;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 3);

      // Tree should be
      // (root (6:3, 4:6 (1:4, 2:4), 2:5))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {4}:6, {2}:5, {1,4}:4, {2,4}:4, {6}:3,
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 5);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->children_nodes[0]->item_count, 6);
    }
    void testTOPKAlgorithm2(void){
      // Filter on min_length
      gl_sarray database = gl_sarray({flex_list{0,1,4},
                                      flex_list{1,2,4,6},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 1;
      size_t top_k = 3;
      size_t min_length = 2;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 3);

      // Tree should be
      // (root (4:6 (1:4, 2:4), 2:5 (6:3)))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {2, 6}:3, {1,4}:4, {2,4}:4,
      // std::cout << closed_itemset;
      TS_ASSERT_EQUALS(closed_itemset.size(), 3);
    }
    void testTOPKAlgorithm3(void){
      // Check it matches CLOSET
      gl_sarray database = gl_sarray({flex_list{0,1,4,6},
                                      flex_list{1,2,4},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 3;
      size_t top_k = TOP_K_MAX;
      size_t min_length = 1;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 3);

      // Tree should be
      // (root (6:3, 4:6 (1:4, 2:4), 2:5))
      //std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {4}:6, {2}:5, {1,4}:4, {2,4}:4, {6}:3,
      //std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 5);

    }
    void testTOPKAlgorithm4(void){
      // Impossible Conditions
      gl_sarray database = gl_sarray({flex_list{0,1,4,6},
                                      flex_list{1,2,4},
                                      flex_list{2,4,6},
                                      flex_list{0,2,6},
                                      flex_list{1,3,4},
                                      flex_list{2,4,5},
                                      flex_list{1,2,3,4}});
      size_t min_support = 4;
      size_t top_k = 5;
      size_t min_length = 3;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 4);

      // Tree should be empty
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be empty
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 0);

      }

    void testTOPKAlgorithm5(void){
      // Repeated Input
      gl_sarray database = gl_sarray({flex_list{0},
                                      flex_list{0,1,1},
                                      flex_list{0,1,2,2},
                                      flex_list{0,1,2,2,2,3,3},
                                      flex_list{0,0,0,1,2,3,4},
                                      flex_list{0,1,2,3,4,5}});
      size_t min_support = 1;
      size_t top_k = 3;
      size_t min_length = 2;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 3);

      // Tree should be
      // (root (0:6 (1:5 (2:4 (3:3 (4:2 (5:1)))))))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {0,1}: 5, {0,1,2}:4, {0,1,2,3}:3
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 3);

      }

    void testTOPKAlgorithm6(void){
      // Undefined Input
      gl_sarray database = gl_sarray({flex_list{0},
                                      flex_list{0,1},
                                      flex_list{0,1,2},
                                      flex_list{0,1,2,3, FLEX_UNDEFINED},
                                      flex_list{0,1,2,3,4},
                                      flex_list{0,1,2,3,4,5},
                                      flex_list{FLEX_UNDEFINED}});
      size_t min_support = 1;
      size_t top_k = 3;
      size_t min_length = 2;

      fp_top_k_results_tree closed_itemset_tree;
      closed_itemset_tree = top_k_algorithm(database, min_support, top_k, min_length);

      TS_ASSERT_EQUALS(closed_itemset_tree.top_k, top_k);
      TS_ASSERT_EQUALS(closed_itemset_tree.min_length, min_length);
      TS_ASSERT_EQUALS(min_support, 3);

      // Tree should be
      // (root (0:6 (1:5 (2:4 (3:3 (4:2 (5:1)))))))
      // std::cout << closed_itemset_tree;

      gl_sframe closed_itemset = closed_itemset_tree.get_closed_itemsets();

      // Closed Itemsets should be
      // {0,1}: 5, {0,1,2}:4, {0,1,2,3}:3
      // std::cout << closed_itemset;

      TS_ASSERT_EQUALS(closed_itemset.size(), 3);
      TS_ASSERT_EQUALS(closed_itemset_tree.root_node->item_count, 7);

      }




};

BOOST_FIXTURE_TEST_SUITE(_fp_growth_test, fp_growth_test)
BOOST_AUTO_TEST_CASE(testCLOSETAlgorithm1) {
  fp_growth_test::testCLOSETAlgorithm1();
}
BOOST_AUTO_TEST_CASE(testCLOSETAlgorithm2) {
  fp_growth_test::testCLOSETAlgorithm2();
}
BOOST_AUTO_TEST_CASE(testCLOSETAlgorithm3) {
  fp_growth_test::testCLOSETAlgorithm3();
}
BOOST_AUTO_TEST_CASE(testCLOSETAlgorithm4) {
  fp_growth_test::testCLOSETAlgorithm4();
}
BOOST_AUTO_TEST_CASE(testCLOSETAlgorithm5) {
  fp_growth_test::testCLOSETAlgorithm5();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm1) {
  fp_growth_test::testTOPKAlgorithm1();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm2) {
  fp_growth_test::testTOPKAlgorithm2();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm3) {
  fp_growth_test::testTOPKAlgorithm3();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm4) {
  fp_growth_test::testTOPKAlgorithm4();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm5) {
  fp_growth_test::testTOPKAlgorithm5();
}
BOOST_AUTO_TEST_CASE(testTOPKAlgorithm6) {
  fp_growth_test::testTOPKAlgorithm6();
}
BOOST_AUTO_TEST_SUITE_END()
