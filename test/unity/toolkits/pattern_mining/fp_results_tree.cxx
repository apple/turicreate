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
#include <toolkits/pattern_mining/fp_results_tree.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::pattern_mining;

/**
 *  Run tests.
*/
struct fp_results_tree_test  {

  public:

    // Test fp_results_tree::fp_results_tree (Constructor)
    void testResultsTreeDefaultConstructor(void){
      fp_results_tree my_results;
      TS_ASSERT(my_results.root_node == nullptr);
      TS_ASSERT(my_results.id_order_map.empty());
      TS_ASSERT(my_results.hash_id_map.empty());
    }
    void testResultsTreeConstruction(void) {
      std::vector<size_t> id_order = {2, 1, 4, 0, 3, 6};
      fp_results_tree my_results = fp_results_tree(id_order);

      // Check root_node
      TS_ASSERT_EQUALS(my_results.root_node->item_id, ROOT_ID);

      // Check id_order_map
      TS_ASSERT_EQUALS(my_results.id_order_map.size(), 6);

      // Check hash_id_map
      TS_ASSERT_EQUALS(my_results.hash_id_map.size(), 6);
      TS_ASSERT(my_results.hash_id_map.find(0) != \
          my_results.hash_id_map.end());
      TS_ASSERT(my_results.hash_id_map[0] == nullptr);
      TS_ASSERT(my_results.hash_id_map.find(9) == \
          my_results.hash_id_map.end());
    }
    void testResultsTreeCopyConstructor(void) {
      std::vector<size_t> id_order = {2, 1, 4, 0, 3, 6};
      fp_results_tree results_one;
      fp_results_tree results_two = fp_results_tree(id_order);
      id_order = {3, 4, 1};
      fp_results_tree results_three = fp_results_tree(id_order);

      // Check Construction
      TS_ASSERT_EQUALS(results_one.id_order_map.size(), 0);
      TS_ASSERT_EQUALS(results_two.id_order_map.size(), 6);
      TS_ASSERT_EQUALS(results_three.id_order_map.size(), 3);

      results_two = results_three;
      TS_ASSERT_EQUALS(results_two.id_order_map.size(), 3);
      results_three = results_one;
      TS_ASSERT_EQUALS(results_three.id_order_map.size(), 0);
    }

    // Test add_itemset()
    void testAddItemset(void) {
      std::vector<size_t> id_order = {2, 1, 4, 0, 5, 3};
      fp_results_tree my_results = fp_results_tree(id_order);

      std::vector<size_t> transaction_1 = {1, 0};
      std::vector<size_t> transaction_2 = {1, 2, 3};
      std::vector<size_t> transaction_3 = {2, 4, 0};

      my_results.add_itemset(transaction_1, 10);
      my_results.add_itemset(transaction_2, 12);
      my_results.add_itemset(transaction_3, 20);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be (root (1:10 (0:10), 2:20 (1:12 (3:12), 4:20 (0:20))))
      std::shared_ptr<fp_node> root_node = my_results.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 10);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->item_count, 20);

      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 0);

      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[0]->item_count, 12);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[1]->item_id, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[1]->item_count, 20);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[1]->depth, 2);

      // Check linked lists
      TS_ASSERT(my_results.hash_id_map[5] == nullptr);

      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->item_id, 0);
      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->item_count, 20);
      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->depth, 3);
      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->next_node->item_id, 0);
      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->next_node->item_count, 10);
      TS_ASSERT_EQUALS(my_results.hash_id_map[0]->next_node->depth, 2);

      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->item_id, 3);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->item_count, 12);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->depth, 3);
      TS_ASSERT(my_results.hash_id_map[3]->next_node == nullptr);

    }

    // test build_tree()
    void testBuildTree1(void){
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{1, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{1, 0}}},
                                {"support", {20, 24, 20, 30, 13}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // std::cout << "my_results" << my_results;
      // my_results should be
      // (root (2:30 (1:20 (4:20), 3:24), 3:20 (1:20), 1:13 (0:13)))

      // Test Structure
      std::shared_ptr<fp_node> root_node = my_results.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 30);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_count, 13);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->depth, 1);

      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 20);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->depth, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[1]->item_id, 3);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[1]->item_count, 24);
    }

    void testBuildTree2(void){
      std::vector<size_t> id_order = {7, 4, 9, 3, 2};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{7},
                                              flex_list{4},
                                              flex_list{9},
                                              flex_list{7, 4},
                                              flex_list{7, 3},
                                              flex_list{7, 4, 9, 3},
                                              flex_list{4, 2}}},
                                {"support", {15, 13, 10, 8, 7, 5, 4}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // std::cout << "my_results" << my_results;
      // my_results should be
      // (root (7:15 (4:8 (9:5 (3:5)), 3:7), 4:13 (2:4), 9:10))

      // Test Structure
      std::shared_ptr<fp_node> root_node = my_results.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 7);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 15);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_id, 9);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_count, 10);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->depth, 1);

      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 8);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->depth, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[1]->item_id, 3);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[1]->item_count, 7);


      // Test Linked Lists
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->item_id, 3);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->item_count, 5);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->depth, 4);

      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->next_node->item_id, 3);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->next_node->item_count, 7);
      TS_ASSERT_EQUALS(my_results.hash_id_map[3]->next_node->depth, 2);
      TS_ASSERT(my_results.hash_id_map[3]->next_node->next_node == nullptr);

    }

    // test prune_tree()
    void testPruneTree(void){
      // Setup
      std::vector<size_t> id_order = {7, 4, 9, 3, 2};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{7},
                                              flex_list{4},
                                              flex_list{9},
                                              flex_list{7, 4},
                                              flex_list{7, 3},
                                              flex_list{7, 4, 9, 3},
                                              flex_list{4, 2}}},
                                {"support", {15, 13, 10, 8, 7, 5, 4}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // std::cout << "my_results" << my_results;
      // my_results should be
      // (root (7:15 (4:8 (9:5 (3:5)), 3:7), 4:13 (2:4), 9:10))

      my_results.prune_tree(8);

      // std::cout << "my_pruned_results" << my_results << std::endl;
      // my_results should now be
      // (root (7:15 (4:8), 4:13, 9:10))

      // Test Structure
      std::shared_ptr<fp_node> root_node = my_results.root_node;
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 7);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 15);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_id, 9);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->item_count, 10);
      TS_ASSERT_EQUALS(root_node->children_nodes[2]->depth, 1);

      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_count, 8);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->depth, 2);

      // Test Linked Lists
      TS_ASSERT(my_results.hash_id_map[3] == nullptr);

      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->item_id, 4);
      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->item_count, 8);
      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->depth, 2);
      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->next_node->item_id, 4);
      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->next_node->item_count, 13);
      TS_ASSERT_EQUALS(my_results.hash_id_map[4]->next_node->depth, 1);
      TS_ASSERT(my_results.hash_id_map[4]->next_node->next_node == nullptr);

    }

    // test get_closed_itemsets()
    void testGetClosedItemsets(void){
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{1, 0}}},
                                {"support", {20, 24, 20, 30, 13}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (2:30 (1:20 (4:20), 3:24), 3:20 (1:20), 1:13 (0:13)))

      gl_sframe itemset_sf = my_results.get_closed_itemsets();
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 5);

    }
    void testGetClosedItemsets2(void){
      std::vector<size_t> id_order = {2, 5, 8, 1, 3};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1, 5},
                                              flex_list{2, 8},
                                              flex_list{2, 5, 8},
                                              flex_list{2},
                                              flex_list{5},
                                              flex_list{2, 5},
                                              flex_list{8, 3}}},
                                {"support", {10, 24, 20, 30, 27, 24, 15}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (2:30 (5:24 (8:20), 8:24), 5:27 (1:10), 8:15 (3:15)))
      gl_sframe itemset_sf = my_results.get_closed_itemsets();
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 7);
    }
    void testGetClosedItemsets3(void){
      std::vector<size_t> id_order = {1, 2, 3, 4, 5};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1},
                                              flex_list{2},
                                              flex_list{1, 2},
                                              flex_list{1, 2, 3, 4},
                                              flex_list{1, 2, 3, 4, 5}}},
                                {"support", {10, 9, 8, 7, 5}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (1:10 (2:8 (3:7 (4:7 (5:5)))), 2:9))

      gl_sframe itemset_sf = my_results.get_closed_itemsets();
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 5);

    }

    // test get_top_k_closed_itemsets()
    void testGetTopKClosedItemsets1(void){
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{1, 0}}},
                                {"support", {20, 24, 20, 30, 13}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (2:30 (1:20 (4:20), 3:24), 3:20 (1:20), 1:13 (0:13)))

      size_t top_k = 3;
      size_t min_length = 2;
      gl_sframe itemset_sf = my_results.get_top_k_closed_itemsets(top_k, min_length);
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 3);

    }
    void testGetTopKClosedItemsets2(void){
      std::vector<size_t> id_order = {2, 5, 8, 1, 3};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1, 5},
                                              flex_list{2, 8},
                                              flex_list{2, 5, 8},
                                              flex_list{2},
                                              flex_list{5},
                                              flex_list{2, 5},
                                              flex_list{8, 3}}},
                                {"support", {10, 24, 20, 30, 27, 24, 15}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (2:30 (5:24 (8:20), 8:24), 5:27 (1:10), 8:15 (3:15)))

      size_t top_k = 10;
      size_t min_length = 2;
      gl_sframe itemset_sf = my_results.get_top_k_closed_itemsets(top_k, min_length);
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 5);
    }
    void testGetTopKClosedItemsets3(void){
      std::vector<size_t> id_order = {1, 2, 3, 4, 5};
      gl_sframe closed_itemsets{{"itemset", { flex_list{1},
                                              flex_list{2},
                                              flex_list{1, 2},
                                              flex_list{1, 2, 3, 4},
                                              flex_list{1, 2, 3, 4, 5}}},
                                {"support", {10, 9, 8, 7, 5}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      // logprogress_stream << "my_results" << my_results;
      // my_results should be
      // (root (1:10 (2:8 (3:7 (4:7 (5:5)))), 2:9))

      size_t top_k = 10;
      size_t min_length = 3;
      gl_sframe itemset_sf = my_results.get_top_k_closed_itemsets(top_k, min_length);
      //std::cout << "closed itemsets" << itemset_sf;

      TS_ASSERT_EQUALS(itemset_sf.num_columns(), 2);
      TS_ASSERT_EQUALS(itemset_sf.size(), 2);

    }

    // Test sort_itemset
    void testSortItemset1(void) {
      // Test 1 (Sorting)
      std::vector<size_t> id_order = {4, 2, 1, 5, 0, 3};
      fp_results_tree my_results = fp_results_tree(id_order);
      std::vector<size_t> itemset = {2, 1, 0, 4};

      std::vector<size_t> sorted_itemset = my_results.sort_itemset(itemset);

      std::vector<size_t> expected_itemset = {4, 2, 1, 0};
      TS_ASSERT_EQUALS(sorted_itemset, expected_itemset);
    }

   void testSortItemset2(void) {
      // Test 2 (Extra elements)
      std::vector<size_t> id_order = {7, 2, 1, 5, 10, 3};
      fp_results_tree my_results = fp_results_tree(id_order);
      std::vector<size_t> itemset = {2, 6, 7, 10, 9, 1};

      std::vector<size_t> sorted_itemset = my_results.sort_itemset(itemset);

      std::vector<size_t> expected_itemset = {7, 2, 1, 10};
      TS_ASSERT_EQUALS(sorted_itemset, expected_itemset);
    }

    // test is_subset_on_path()
    void testIsSubsetOnPath(void) {
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{1, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{1, 0}}},
                                {"support", {20, 24, 20, 30, 13}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);
      // my_results should be
      // (root (2:30 (1:20 (4:20), 3:24), 3:20 (1:20), 1:13 (0:13)))

      std::vector<size_t> sorted_itemset = {2, 1};

      fp_node* head_node = my_results.hash_id_map[1];
      // path from head_node to root_node should be (1 -> Root)
      TS_ASSERT_EQUALS(is_subset_on_path(sorted_itemset, head_node), false);

      head_node = head_node->next_node;
      // path from head_node to root_node should be (1 -> 3 -> Root)
      TS_ASSERT_EQUALS(is_subset_on_path(sorted_itemset, head_node), false);

      head_node = head_node->next_node;
      // path from head_node to root_node should be (1 -> 2 -> Root)
      TS_ASSERT_EQUALS(is_subset_on_path(sorted_itemset, head_node), true);
    }

    // test is_itemset_redundant
    void testIsItemsetRedundant1(void) {
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{1, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{1, 0}}},
                                {"support", {20, 24, 20, 30, 13}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);
      // my_results should be
      // (root (2:30 (1:20 (4:20), 3:24), 3:20 (1:20), 1:13 (0:13)))

      std::vector<size_t> potential_itemset = {2, 1};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 25), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 20), true);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 10), true);

      potential_itemset = {2, 4};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 25), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 20), true);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 10), true);

      potential_itemset = {2, 1, 3};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 25), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 20), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 10), false);

      potential_itemset = {};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 25), true);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 20), true);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 10), true);
    }

   void testIsItemsetRedundant2(void) {
      std::vector<size_t> id_order = {5, 3, 4, 2, 0, 1};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{5, 2, 4},
                                              flex_list{2, 3},
                                              flex_list{3, 1},
                                              flex_list{3},
                                              flex_list{4, 5},
                                              flex_list{5, 0}}},
                                {"support", {20, 14, 10, 18, 26, 12}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);
      // my_results should be
      // (root (5:26, (4:26 (2:20), 0:12), 3:18 (2:14, 1:10)))

      std::vector<size_t> potential_itemset = {5};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 27), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 26), true);

      potential_itemset = {2, 3};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 14), true);

      potential_itemset = {0};
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 14), false);
      TS_ASSERT_EQUALS(my_results.is_itemset_redundant(potential_itemset, 12), true);

    }

    // test fp_top_k_results_tree::insert_support() and get_min_support_bound()
    void testFPTopKSupportHeap1(void){
      fp_top_k_results_tree my_results;
      my_results.top_k = 5;

      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 0);

      my_results.insert_support(2);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      my_results.insert_support(3);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 2);

      my_results.insert_support(5);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      my_results.insert_support(5);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 4);

      my_results.insert_support(6);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 2);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 5);

      my_results.insert_support(6);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 3);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 5);

      my_results.insert_support(2);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 3);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 5);

      my_results.insert_support(4);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 4);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 5);
   }
   void testFPTopKSupportHeap2(void){
      fp_top_k_results_tree my_results;
      my_results.top_k = 3;

      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 0);

      my_results.insert_support(2);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      my_results.insert_support(3);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 2);

      my_results.insert_support(3);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 2);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      my_results.insert_support(6);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 3);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      my_results.insert_support(2);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 3);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      my_results.insert_support(4);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 3);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      my_results.insert_support(4);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 4);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);
   }

   void testFPTopKAddItemset1(void){
      // Default Test
      std::vector<size_t> id_order = {3, 2, 9, 0, 8};
      size_t k = 3;
      size_t len = 2;
      fp_top_k_results_tree my_results = fp_top_k_results_tree(id_order, k, len);


      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 0);

      std::vector<size_t> transaction_1 = {2, 0};
      my_results.add_itemset(transaction_1, 10);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      std::vector<size_t> transaction_2 = {3, 9, 8};
      my_results.add_itemset(transaction_2, 12);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 2);

      std::vector<size_t> transaction_3 = {3, 2, 0};
      my_results.add_itemset(transaction_3, 20);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 10);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      std::vector<size_t> transaction_4 = {9, 0};
      my_results.add_itemset(transaction_4, 13);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 12);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);
    }

   void testFPTopKAddItemset2(void){
      // Filter on Length
      std::vector<size_t> id_order = {3, 2, 9, 0, 8};
      size_t k = 3;
      size_t len = 2;
      fp_top_k_results_tree my_results = fp_top_k_results_tree(id_order, k, len);


      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 0);

      std::vector<size_t> transaction_1 = {2, 0};
      my_results.add_itemset(transaction_1, 10);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      std::vector<size_t> transaction_2 = {3};
      my_results.add_itemset(transaction_2, 32);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      std::vector<size_t> transaction_3 = {3, 2, 0};
      my_results.add_itemset(transaction_3, 20);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 2);

      std::vector<size_t> transaction_4 = {3, 9, 8};
      my_results.add_itemset(transaction_4, 15);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 10);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);

      std::vector<size_t> transaction_5 = {2};
      my_results.add_itemset(transaction_5, 18);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 10);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);
    }

    void testFPTopKAddItemset3(void){
      // Subset/Superset Test
      std::vector<size_t> id_order = {3, 2, 9, 0, 8};
      size_t k = 3;
      size_t len = 1;
      fp_top_k_results_tree my_results = fp_top_k_results_tree(id_order, k, len);


      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 0);

      std::vector<size_t> transaction_1 = {3, 2};
      my_results.add_itemset(transaction_1, 10);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      std::vector<size_t> transaction_2 = {3, 2, 9};
      my_results.add_itemset(transaction_2, 10);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 1);

      std::vector<size_t> transaction_3 = {3, 2, 9, 0};
      my_results.add_itemset(transaction_3, 8);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 1);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 2);

      std::vector<size_t> transaction_4 = {3};
      my_results.add_itemset(transaction_4, 18);
      TS_ASSERT_EQUALS(my_results.get_min_support_bound(), 8);
      TS_ASSERT_EQUALS(my_results.min_support_heap.size(), 3);
    }

    // Test Get Support
    void testGetSupport(void){
      std::vector<size_t> id_order = {7, 4, 9, 3, 2};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{7},
                                              flex_list{4},
                                              flex_list{9},
                                              flex_list{7, 4},
                                              flex_list{7, 3},
                                              flex_list{7, 4, 9, 3},
                                              flex_list{4, 2}}},
                                {"support", {15, 13, 10, 8, 7, 5, 4}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      std::vector<size_t> itemset = {7};
      TS_ASSERT_EQUALS(my_results.get_support(itemset), 15);
      TS_ASSERT_EQUALS(my_results.get_support(itemset, 10), 15);
      TS_ASSERT_EQUALS(my_results.get_support(itemset, 18), 18);

      itemset = {4, 3};
      TS_ASSERT_EQUALS(my_results.get_support(itemset), 5);
      TS_ASSERT_EQUALS(my_results.get_support(itemset, 3), 5);
      TS_ASSERT_EQUALS(my_results.get_support(itemset, 9), 9);

      itemset = {9, 2};
      TS_ASSERT_EQUALS(my_results.get_support(itemset), 0);
      TS_ASSERT_EQUALS(my_results.get_support(itemset, 5), 5);

    }


};


BOOST_FIXTURE_TEST_SUITE(_fp_results_tree_test, fp_results_tree_test)
BOOST_AUTO_TEST_CASE(testResultsTreeDefaultConstructor) {
  fp_results_tree_test::testResultsTreeDefaultConstructor();
}
BOOST_AUTO_TEST_CASE(testResultsTreeConstruction) {
  fp_results_tree_test::testResultsTreeConstruction();
}
BOOST_AUTO_TEST_CASE(testResultsTreeCopyConstructor) {
  fp_results_tree_test::testResultsTreeCopyConstructor();
}
BOOST_AUTO_TEST_CASE(testAddItemset) {
  fp_results_tree_test::testAddItemset();
}
BOOST_AUTO_TEST_CASE(testBuildTree1) {
  fp_results_tree_test::testBuildTree1();
}
BOOST_AUTO_TEST_CASE(testBuildTree2) {
  fp_results_tree_test::testBuildTree2();
}
BOOST_AUTO_TEST_CASE(testPruneTree) {
  fp_results_tree_test::testPruneTree();
}
BOOST_AUTO_TEST_CASE(testGetClosedItemsets) {
  fp_results_tree_test::testGetClosedItemsets();
}
BOOST_AUTO_TEST_CASE(testGetClosedItemsets2) {
  fp_results_tree_test::testGetClosedItemsets2();
}
BOOST_AUTO_TEST_CASE(testGetClosedItemsets3) {
  fp_results_tree_test::testGetClosedItemsets3();
}
BOOST_AUTO_TEST_CASE(testGetTopKClosedItemsets1) {
  fp_results_tree_test::testGetTopKClosedItemsets1();
}
BOOST_AUTO_TEST_CASE(testGetTopKClosedItemsets2) {
  fp_results_tree_test::testGetTopKClosedItemsets2();
}
BOOST_AUTO_TEST_CASE(testGetTopKClosedItemsets3) {
  fp_results_tree_test::testGetTopKClosedItemsets3();
}
BOOST_AUTO_TEST_CASE(testSortItemset1) {
  fp_results_tree_test::testSortItemset1();
}
BOOST_AUTO_TEST_CASE(testSortItemset2) {
  fp_results_tree_test::testSortItemset2();
}
BOOST_AUTO_TEST_CASE(testIsSubsetOnPath) {
  fp_results_tree_test::testIsSubsetOnPath();
}
BOOST_AUTO_TEST_CASE(testIsItemsetRedundant1) {
  fp_results_tree_test::testIsItemsetRedundant1();
}
BOOST_AUTO_TEST_CASE(testIsItemsetRedundant2) {
  fp_results_tree_test::testIsItemsetRedundant2();
}
BOOST_AUTO_TEST_CASE(testFPTopKSupportHeap1) {
  fp_results_tree_test::testFPTopKSupportHeap1();
}
BOOST_AUTO_TEST_CASE(testFPTopKSupportHeap2) {
  fp_results_tree_test::testFPTopKSupportHeap2();
}
BOOST_AUTO_TEST_CASE(testFPTopKAddItemset1) {
  fp_results_tree_test::testFPTopKAddItemset1();
}
BOOST_AUTO_TEST_CASE(testFPTopKAddItemset2) {
  fp_results_tree_test::testFPTopKAddItemset2();
}
BOOST_AUTO_TEST_CASE(testFPTopKAddItemset3) {
  fp_results_tree_test::testFPTopKAddItemset3();
}
BOOST_AUTO_TEST_CASE(testGetSupport) {
  fp_results_tree_test::testGetSupport();
}
BOOST_AUTO_TEST_SUITE_END()
