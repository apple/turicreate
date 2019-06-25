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
#include <toolkits/pattern_mining/fp_tree.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::pattern_mining;

/**
 *  Run tests.
*/
struct fp_tree_test  {

  public:
    // Test fp_tree:fp_tree (Constructor)
    void testFPTreeDefaultConstructor(void){
      fp_tree my_tree;
      TS_ASSERT(my_tree.root_node == nullptr);
      TS_ASSERT_EQUALS(my_tree.header.size(), 0);
    }
    void testFPTreeConstructor(void) {
      std::vector<size_t> ids = {2, 1, 4, 0};
      std::vector<size_t> supports = {4, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      fp_tree my_tree = fp_tree(header);

      // Check root_node
      TS_ASSERT_EQUALS(my_tree.root_node->item_id, ROOT_ID);

      // Check header_ids
      auto header_ids = my_tree.header.get_ids();
      TS_ASSERT_EQUALS(header_ids.size(), 4);
      TS_ASSERT_EQUALS(header_ids, ids);

      // Check header_supports
      auto header_supports = my_tree.header.get_supports();
      TS_ASSERT_EQUALS(header_supports, supports);

      // Check header_pointers
      auto header_pointers = my_tree.header.get_pointers();
      TS_ASSERT_EQUALS(header_pointers.size(), 4);
      TS_ASSERT(header_pointers.find(0) != header_pointers.end());
      TS_ASSERT(header_pointers[0] == nullptr);
      TS_ASSERT(header_pointers.find(3) == header_pointers.end());
    }
    void testFPTreeCopyConstructor(void) {
      fp_tree tree_one;

      std::vector<size_t> ids = {2, 1, 4, 0};
      std::vector<size_t> supports = {4, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      fp_tree tree_two = fp_tree(header);

      ids = {3, 4, 1};
      supports = {99, 88, 77};
      header = fp_tree_header(ids, supports);
      fp_tree tree_three = fp_tree(header);

      // Check Construction
      TS_ASSERT_EQUALS(tree_one.header.size(), 0);
      TS_ASSERT_EQUALS(tree_two.header.size(), 4);
      TS_ASSERT_EQUALS(tree_three.header.size(), 3);

      tree_two = tree_three;
      TS_ASSERT_EQUALS(tree_two.header.get_ids(), std::vector<size_t>({3, 4, 1}));
      tree_three = tree_one;
      TS_ASSERT_EQUALS(tree_three.header.get_ids(), std::vector<size_t>({}));
    }

    // Test fp_tree:add_transaction
    void testAddTransaction(void) {
      std::vector<size_t> ids = {2, 1, 4, 0, 5};
      std::vector<size_t> supports = {4, 3, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      fp_tree my_tree = fp_tree(header);

      std::vector<size_t> transaction_1 = {1, 0};
      std::vector<size_t> transaction_2 = {1, 2, 3};
      std::vector<size_t> transaction_3 = {2, 4, 0};

      my_tree.add_transaction(transaction_1, 1);
      my_tree.add_transaction(transaction_2, 1);
      my_tree.add_transaction(transaction_3, 2);

      // my_tree should be (root:0 (1:1 (0:1), 2:3 (1:1, 4:2 (0:2))))
      std::shared_ptr<fp_node> root_node = my_tree.root_node;
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 4);
      TS_ASSERT_EQUALS(root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->item_count, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->item_id, 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->item_count, 3);

      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->children_nodes[0]->item_id, 0);

      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[0]->item_count, 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[1]->item_id, 4);
      TS_ASSERT_EQUALS(root_node->children_nodes[1]->children_nodes[1]->item_count, 2);

      // Explore Header List
      auto header_pointers = my_tree.header.get_pointers();
      TS_ASSERT(header_pointers[5] == nullptr);
      TS_ASSERT(header_pointers[4]);
      TS_ASSERT(header_pointers[4]->next_node == nullptr);
      TS_ASSERT(header_pointers[0]);
      TS_ASSERT(header_pointers[0]->next_node);
      TS_ASSERT(header_pointers[1]);
      TS_ASSERT(header_pointers[1]->next_node);
    }

    // Test for get_item_counts()
    void testGetItemCounts(void){
      // Database
      gl_sarray database({flex_list{0,1,4},
                          flex_list{1,2,4},
                          flex_list{3,4},
                          flex_list{0,2},
                          flex_list{1,4}});

      std::vector<std::pair<size_t, size_t>> item_counts = \
        get_item_counts(database);
      std::vector<std::pair<size_t, size_t>> expected_item_counts = \
        {{0, 2}, {1, 3}, {2, 2}, {3, 1}, {4, 4}};
      // item_counts should be
      //    { 0:2, 1:3, 2:2, 3:1, 4:4 }
      TS_ASSERT_EQUALS(item_counts, expected_item_counts);

    }

    // Test build_header()
    void testBuildHeader_1(void) {
      // Test 1 (Sorting)
      std::vector<std::pair<size_t, size_t>> item_counts = \
        {{0, 2}, {1, 4}, {2, 1}, {3, 10}};
      size_t min_support = 1;
      auto header = build_header(item_counts, min_support);
      std::vector<size_t> expected_ids = {3, 1, 0, 2};
      TS_ASSERT_EQUALS(header.get_ids(), expected_ids);
      std::vector<size_t> expected_supports = {10, 4, 2, 1};
      TS_ASSERT_EQUALS(header.get_supports(), expected_supports);
    }

    void testBuildHeader_2(void) {
      // Test 2 (Ties)
      std::vector<std::pair<size_t, size_t>> item_counts = \
        {{0, 4}, {1, 4}, {2, 1}, {3, 3}};
      size_t min_support = 1;
      auto header = build_header(item_counts, min_support);
      std::vector<size_t> expected_ids = {0, 1, 3, 2};
      TS_ASSERT_EQUALS(header.get_ids(), expected_ids);
      std::vector<size_t> expected_supports = {4, 4, 3, 1};
      TS_ASSERT_EQUALS(header.get_supports(), expected_supports);
     }

    void testBuildHeader_3(void) {
      // Test 3 (Filtering)
      std::vector<std::pair<size_t, size_t>> item_counts = \
        {{0, 4}, {1, 6}, {2, 1}, {3, 3}};
      size_t min_support = 2;
      auto header = build_header(item_counts, min_support);
      std::vector<size_t> expected_ids = {1, 0, 3};
      TS_ASSERT_EQUALS(header.get_ids(), expected_ids);
      std::vector<size_t> expected_supports = {6, 4, 3};
      TS_ASSERT_EQUALS(header.get_supports(), expected_supports);
     }

    void testBuildHeader_4(void) {
      // Test 4 (Unordered)
      std::vector<std::pair<size_t, size_t>> item_counts = \
        {{5, 14}, {32, 4}, {2, 12},{13, 33}, {9, 6}, {4, 12}};
      size_t min_support = 10;
      auto header = build_header(item_counts, min_support);
      std::vector<size_t> expected_ids = {13, 5, 2, 4};
      TS_ASSERT_EQUALS(header.get_ids(), expected_ids);
      std::vector<size_t> expected_supports = {33, 14, 12, 12};
      TS_ASSERT_EQUALS(header.get_supports(), expected_supports);
     }

    // Test flex_to_id_vector()
    void testFlexToIdVector(void) {
      flexible_type transaction_array = flex_list{0, 3, 5};
      std::vector<size_t> expected_vector = {0, 3, 5};
      TS_ASSERT_EQUALS(flex_to_id_vector(transaction_array), expected_vector);

      transaction_array = flex_list{8, 1, 4, 3};
      expected_vector = {1, 3, 4, 8};
      TS_ASSERT_EQUALS(flex_to_id_vector(transaction_array), expected_vector);

      transaction_array = flex_list{};
      expected_vector = {};
      TS_ASSERT_EQUALS(flex_to_id_vector(transaction_array), expected_vector);

      transaction_array = flex_list{1,1,3};
      expected_vector = {1,3};
      TS_ASSERT_EQUALS(flex_to_id_vector(transaction_array), expected_vector);

    }

    // Test build_tree()
    void testBuildTree_1(void) {
      // Test 1 - Build tree
      gl_sarray database = gl_sarray({flex_list{0,1,4},
                                      flex_list{1,2,4},
                                      flex_list{3,4},
                                      flex_list{0,2},
                                      flex_list{1,4}});
      size_t min_support = 1;
      fp_tree my_tree = build_tree(database, min_support);

      // Tree should be (root (4:4 (1:3 (0:1, 2:1), 3:1), 0:1 (2:1)))

      // Check header_ids
      std::vector<size_t> expected_header_ids = {4, 1, 0, 2, 3};
      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);

      // Check Tree Structure
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 5);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_count, 4);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]->item_id, 0);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]->item_count, 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_id, 3);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_count, 1);

      // Check header_pointers lists
      auto header_pointers = my_tree.header.get_pointers();
      TS_ASSERT_EQUALS(header_pointers.size(), 5);
      TS_ASSERT_EQUALS(header_pointers[4]->item_id, 4);
      TS_ASSERT_EQUALS(header_pointers[0]->item_id, 0);
      TS_ASSERT_EQUALS(header_pointers[0]->next_node->item_id, 0);
    }

    void testBuildTree_2(void) {
      // Test 2 - Prune Transactions
      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
                                      flex_list{4, 1, 5},
                                      flex_list{9, 2},
                                      flex_list{5, 4, 3},
                                      flex_list{2, 4},
                                      flex_list{1, 4}});
      size_t min_support = 2;
      fp_tree my_tree = build_tree(database, min_support);

      // Tree should be (root (4:5 (2:2 (5:1), 5:2 (1:1), 1:1), 2:1))

      // Check header_ids
      std::vector<size_t> expected_header_ids = {4, 2, 5, 1};
      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);

      // Check Tree Structure
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 6);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_count, 5);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes.size(), 3);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_id, 5);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_count, 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->children_nodes.size(), 1);

      // Check header_pointers lists
      auto header_pointers = my_tree.header.get_pointers();
      TS_ASSERT_EQUALS(header_pointers.size(), 4);
      TS_ASSERT_EQUALS(header_pointers[4]->item_id, 4);
      TS_ASSERT_EQUALS(header_pointers[2]->item_id, 2);
      TS_ASSERT_EQUALS(header_pointers[2]->next_node->item_id, 2);

    }

    void testBuildTree_3(void) {
      // Test 3 - Empty Tree
      gl_sarray database3 = gl_sarray({flex_list{1},
                                       flex_list{3},
                                       flex_list{10}});
      size_t min_support = 100;
      fp_tree my_tree = build_tree(database3, min_support);

      // Tree should be empty
     std::vector<size_t> expected_header_ids = {};
      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 0);
      TS_ASSERT_EQUALS(my_tree.header.size(), 0);
    }

    // Test Prune Tree
    fp_tree setupPruneTree(void) {
      // Build Tree
      gl_sarray database = gl_sarray({flex_list{0,1,4},
                                      flex_list{1,2,4},
                                      flex_list{3,4},
                                      flex_list{6,2,4},
                                      flex_list{1,4},
                                      flex_list{5,2},
                                      flex_list{3,5},
                                      flex_list{0,1,4},
                                      flex_list{1,4}});
      size_t min_support = 1;
      fp_tree my_tree = build_tree(database, min_support);
      // Tree should be (root (4:7 (1:5 (0:2, 2:1), 3:1, 2:1 (6:1)), 2:1 (5:1), 3:1 (5:1)))
      //   header (4:7, 1:5, 2:3, 0:2, 3:2, 5:2, 6:1

      return my_tree;
    }
    void testPruneTree1(void) {
      // Prune Nothing
      fp_tree my_tree = setupPruneTree();
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);

      my_tree.prune_tree(1);
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 3);
    }

    void testPruneTree2(void) {
      // Prune last element
      fp_tree my_tree = setupPruneTree();
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->\
          children_nodes[2]->children_nodes.size(), 1);

      my_tree.prune_tree(2);
      TS_ASSERT_EQUALS(my_tree.header.size(), 6);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 3);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->\
          children_nodes[2]->children_nodes.size(), 0);
    }

    void testPruneTree3(void) {
      // Prune multiple levels
      fp_tree my_tree = setupPruneTree();
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);

      my_tree.prune_tree(4);
      TS_ASSERT_EQUALS(my_tree.header.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->\
          children_nodes.size(), 1);
    }

    void testPruneTree4(void) {
      // Prune everything - Not an expected case
      fp_tree my_tree = setupPruneTree();
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);

      my_tree.prune_tree(100);
      TS_ASSERT_EQUALS(my_tree.header.size(), 0);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 0);
    }

   void testPruneTree5(void) {
      // Repeat Prunes
      fp_tree my_tree = setupPruneTree();
      TS_ASSERT_EQUALS(my_tree.header.size(), 7);

      my_tree.prune_tree(3);
      TS_ASSERT_EQUALS(my_tree.header.size(), 3);

      my_tree.prune_tree(2);
      TS_ASSERT_EQUALS(my_tree.header.size(), 3);
    }


    // Test get_support()
    void testGetSupport(void) {
      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
                                      flex_list{4, 1, 5},
                                      flex_list{9, 2},
                                      flex_list{5, 4, 3},
                                      flex_list{2, 4},
                                      flex_list{1, 4}});
      size_t min_support = 2;
      fp_tree my_tree = build_tree(database, min_support);

      // Tree should be (root (4:5 (2:2 (5:1), 5:2 (1:1), 1:1), 2:1))

      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(2)), 3);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(4)), 5);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(100)), 0);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(3)), 0); // Because min_support is 2

    }
    void testGetSupport2(void) {
      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
                                      flex_list{4, 1, 5},
                                      flex_list{9, 2},
                                      flex_list{5, 4, 3},
                                      flex_list{2, 4},
                                      flex_list{1, 4}});
      size_t min_support = 2;
      fp_tree my_tree = build_tree(database, min_support);

      // Tree should be (root (4:5 (2:2 (5:1), 5:2 (1:1), 1:1), 2:1))

      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(2), 1), 3);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(2), 2), 2);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(2), 3), 0);

      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(5), 1), 3);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(5), 2), 3);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(5), 3), 1);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(5), 4), 0);

      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(3), 1), 0);
      TS_ASSERT_EQUALS(my_tree.get_support(my_tree.header.get_heading(3), 4), 0);

    }

    // Test get_num_transactions()
    void testGetNumTransactions(void) {
      std::vector<size_t> ids = {2, 1, 4, 0, 5};
      std::vector<size_t> supports = {10, 9, 8, 7, 6};
      fp_tree_header header = fp_tree_header(ids, supports);
      fp_tree my_tree = fp_tree(header);
      std::vector<size_t> transaction;

      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 0);

      transaction = {1, 0};
      my_tree.add_transaction(transaction, 2);
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 2);

      transaction = {2, 4};
      my_tree.add_transaction(transaction, 1);
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 3);

      transaction = {2, 1};
      my_tree.add_transaction(transaction, 5);
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 8);

      transaction = {0, 1, 2, 4, 5};
      my_tree.add_transaction(transaction, 1);
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 9);

      transaction = {3};
      my_tree.add_transaction(transaction, 2);
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 11);

    }

    // Test get_supports_at_depth()
    void testGetSupportsAtDepth(void){
      gl_sarray database = gl_sarray({flex_list{2, 4, 5, 8},
                                      flex_list{4, 1, 5, 8},
                                      flex_list{9, 2, 6},
                                      flex_list{5, 4, 3},
                                      flex_list{2, 4, 6, 8},
                                      flex_list{1, 4, 8}});
      size_t min_support = 2;
      fp_tree my_tree = build_tree(database, min_support);

      // Header is (4:5, 8:4, 2:3, 5:3, 1:2, 6:2)
      // Tree should be (root (4:5 (8:4 (2:2 (5:1, 6:1), 5:1 (1:1), 1:1), 5:1), 2:1 (6:1)))

      std::vector<size_t> expected_supports = {5, 4, 3, 3, 2, 2};
      TS_ASSERT_EQUALS(my_tree.get_supports_at_depth(1), expected_supports);

      expected_supports = {0, 4, 2, 3, 2, 2};
      TS_ASSERT_EQUALS(my_tree.get_supports_at_depth(2), expected_supports);

      expected_supports = {0, 0, 2, 2, 2, 1};
      TS_ASSERT_EQUALS(my_tree.get_supports_at_depth(3), expected_supports);

      expected_supports = {0, 0, 0, 1, 1, 1};
      TS_ASSERT_EQUALS(my_tree.get_supports_at_depth(4), expected_supports);

    }

    // Test get_descendant_supports()
    void testGetDescendantSupports(void) {
      gl_sarray database = gl_sarray({flex_list{2, 4, 5, 8},
                                      flex_list{4, 1, 5, 8},
                                      flex_list{9, 2, 6},
                                      flex_list{5, 4, 3},
                                      flex_list{2, 4, 6, 8},
                                      flex_list{1, 4, 8}});
      size_t min_support = 2;
      fp_tree my_tree = build_tree(database, min_support);

      // Header is (4:5, 8:4, 2:3, 5:3, 1:2, 6:2)
      // Tree should be (root (4:5 (8:4 (2:2 (5:1, 6:1), 5:1 (1:1), 1:1), 5:1), 2:1 (6:1)))

      fp_node* anchor_node = my_tree.header.headings[0].pointer;
      TS_ASSERT_EQUALS(anchor_node->item_id, 4);
      std::vector<size_t> expected_supports = {2, 2, 3, 1, 4};
      TS_ASSERT_EQUALS(my_tree.get_descendant_supports(anchor_node), expected_supports);

      anchor_node = my_tree.header.headings[1].pointer;
      TS_ASSERT_EQUALS(anchor_node->item_id, 8);
      expected_supports = {2, 2, 2, 1};
      TS_ASSERT_EQUALS(my_tree.get_descendant_supports(anchor_node), expected_supports);


    }

//    // Test update_closed_node_count()
//    void testUpdateClosedNodeCount1(void) {
//      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
//                                      flex_list{4, 1, 5},
//                                      flex_list{9, 2},
//                                      flex_list{5, 4, 3},
//                                      flex_list{2, 4},
//                                      flex_list{1, 4}});
//      size_t min_support = 1;
//      fp_top_k_tree my_tree = build_top_k_tree(database, min_support);
//      // my_tree should be
//      // (root (4:5 (2:2 (5:1), 5:2 (1:1, 3:1), 1:1), 2:1 (9:1)))
//
//      my_tree.update_closed_node_count();
//      // my_tree.closed_node_count should be
//      // { 5:1, 2:2, 1:5}
//
//      TS_ASSERT_EQUALS(my_tree.closed_node_count.size(), 3);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[5], 1);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 2);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 5);
//
//      my_tree.min_length = 2;
//      my_tree.update_closed_node_count();
//      // closed_node_count should be
//      // { 2:2, 1:5}
//
//      TS_ASSERT_EQUALS(my_tree.closed_node_count.size(), 2);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 2);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 5);
//    }
//    void testUpdateClosedNodeCount2(void) {
//      gl_sarray database = gl_sarray({flex_list{2, 4, 5, 1},
//                                      flex_list{4, 1, 5},
//                                      flex_list{3, 1},
//                                      flex_list{5, 4, 3},
//                                      flex_list{2, 4, 1},
//                                      flex_list{9, 4, 3}});
//      size_t min_support = 1;
//      fp_top_k_tree my_tree = build_top_k_tree(database, min_support);
//      // my_tree should be
//      // (root (4:5 (1:3 (5:2 (2:1), 2:1), 3:2 (5:1, 9:1)), 1:1 (3:1)))
//
//      my_tree.update_closed_node_count();
//      // closed_node_count should be
//      // { 5:1, 1:3, 2:2, 1:5}
//
//      TS_ASSERT_EQUALS(my_tree.closed_node_count.size(), 4);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[5], 1);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[3], 1);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 2);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 5);
//
//      my_tree.min_length = 3;
//      my_tree.update_closed_node_count();
//      // closed_node_count should be
//      // { 2:1, 1:4}
//
//      TS_ASSERT_EQUALS(my_tree.closed_node_count.size(), 2);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 1);
//      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 4);
//    }

    // Test get_cond_item_counts
    void testGetCondItemCounts(void) {
      gl_sarray database = gl_sarray({flex_list{3, 1, 9},
                                      flex_list{4, 1, 8},
                                      flex_list{9, 2},
                                      flex_list{8, 9, 3},
                                      flex_list{2, 4},
                                      flex_list{1, 2, 9}});
      size_t min_support = 1;
      fp_tree my_tree = build_tree(database, min_support);

      // header_ids is {9, 1, 2, 3, 4, 8}

      std::vector<std::pair<size_t, size_t>> item_counts;
      std::vector<std::pair<size_t, size_t>> expected_item_counts;

      auto item3 = my_tree.header.get_heading(3);
      item_counts = my_tree.get_cond_item_counts(item3);
      expected_item_counts = {{9, 2}, {1, 1}, {2, 0}};
      TS_ASSERT_EQUALS(item_counts, expected_item_counts);

      auto item8 = my_tree.header.get_heading(8);
      item_counts = my_tree.get_cond_item_counts(item8);
      expected_item_counts = {{9, 1}, {1, 1}, {2, 0}, {3, 1}, {4, 1}};
      TS_ASSERT_EQUALS(item_counts, expected_item_counts);

      auto item9 = my_tree.header.get_heading(9);
      item_counts = my_tree.get_cond_item_counts(item9);
      expected_item_counts = {};
      TS_ASSERT_EQUALS(item_counts, expected_item_counts);

      auto item77 = my_tree.header.get_heading(77);
      item_counts = my_tree.get_cond_item_counts(item77);
      expected_item_counts = {};
      TS_ASSERT_EQUALS(item_counts, expected_item_counts);

    }

    void testTopKGetMinSupportBound(void) {
      std::map<size_t, size_t> my_map = {{4, 2}, {3, 2}, {2, 6}, {1, 10}};
      fp_top_k_tree my_tree;
      my_tree.closed_node_count = my_map;

      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 1);

      my_tree.top_k = 1;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 4);

      my_tree.top_k = 2;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 4);

      my_tree.top_k = 3;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 3);

      my_tree.top_k = 6;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 2);

      my_tree.top_k = 10;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 2);

      my_tree.top_k = 11;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 1);

      my_tree.top_k = 100;
      TS_ASSERT_EQUALS(my_tree.get_min_support_bound(), 1);
    }

    // Test fp_top_k_tree build_tree()
    void testBuildTopKTree_1(void) {
      // Test 1 - Top K
      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
                                      flex_list{4, 1, 5},
                                      flex_list{3, 2},
                                      flex_list{5, 4, 9},
                                      flex_list{2, 4},
                                      flex_list{1, 4, 7}});
      size_t min_support = 1;
      size_t top_k = 3;
      fp_top_k_tree my_tree = build_top_k_tree(database, min_support, top_k);

      // Full tree should be (root (4:5 (2:2 (5:1), 5:2 (1:1, 9:1), 1:1 (7:1)), 2:1 (3:1)))
      // The mined tree should be (root (4:5 (2:2 (5:1), 5:2(1:1), 1:1), 2:1))

      TS_ASSERT_EQUALS(min_support, 2);

      // Check header_ids
      std::vector<size_t> expected_header_ids = {4, 2, 5, 1};
      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);

      // Check Tree Structure
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 6);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_id, 4);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_count, 5);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes.size(), 3);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_id, 5);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->item_count, 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->children_nodes.size(), 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->children_nodes[0]->item_id, 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]-> \
          children_nodes[1]->children_nodes[0]->item_count, 1);

    }
    void testBuildTopKTree_2(void) {
      // Test 2 - Top K
      gl_sarray database = gl_sarray({flex_list{2, 4, 5},
                                      flex_list{4, 1, 5},
                                      flex_list{3, 2},
                                      flex_list{5, 4, 9},
                                      flex_list{2, 4},
                                      flex_list{1, 4, 7}});
      size_t min_support = 1;
      size_t min_length = 2;
      size_t top_k = 3;
      fp_top_k_tree my_tree = build_top_k_tree(database, min_support, top_k, min_length);

      // Full tree should be (root (4:5 (2:2 (5:1), 5:2 (1:1, 9:1), 1:1 (7:1)), 2:1 (3:1)))
      // The mined tree should be the same

      TS_ASSERT_EQUALS(min_support, 1);

      // Check header_ids
      std::vector<size_t> expected_header_ids = {4, 2, 5, 1, 3, 7, 9};
      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);

      // Check Tree Structure
      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 6);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]->item_id, 2);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]->item_count, 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]-> \
          children_nodes.size(), 1);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]-> \
          children_nodes[0]->item_id, 3);
      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[1]-> \
          children_nodes[0]->item_count, 1);
    }
//    void testBuildTopKTree_3(void) {
//      // Test 3 - Ignore repeats
//      gl_sarray database = gl_sarray({flex_list{1, 2},
//                                      flex_list{1},
//                                      flex_list{1,1,1}});
//      size_t min_support = 1;
//      size_t min_length = 1;
//      size_t top_k = 2;
//      fp_top_k_tree my_tree = build_top_k_tree(database, min_support, top_k, min_length);
//
//      std::cout << my_tree;
//      // Full tree should be (root (1:3 (2:1)))
//      // The mined tree should be the same
//
//      TS_ASSERT_EQUALS(min_support, 1);
//
//      // Check header_ids
//      std::vector<size_t> expected_header_ids = {1, 2};
//      TS_ASSERT_EQUALS(my_tree.header.get_ids(), expected_header_ids);
//
//      // Check Tree Structure
//      TS_ASSERT_EQUALS(my_tree.get_num_transactions(), 3);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes.size(), 1);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_id, 1);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->item_count, 3);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->children_nodes.size(), 1);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->children_nodes[0]->item_id, 2);
//      TS_ASSERT_EQUALS(my_tree.root_node->children_nodes[0]->children_nodes[0]->item_count, 1);
//    }

    // Test fp_top_k_tree::add_transaction()
     void testTopKAddTransaction_1(void) {
      // Check that closed_node_count is updated
      std::vector<size_t> ids = {2, 1, 4, 0, 5};
      std::vector<size_t> supports = {4, 3, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      size_t top_k = 3;
      size_t min_length = 1;

      fp_top_k_tree my_tree = fp_top_k_tree(header, top_k, min_length);


      std::vector<size_t> transaction_1 = {1, 0};
      my_tree.add_transaction(transaction_1, 1);
      // my_tree should be (root (1:1 (0:1)))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 1);

      std::vector<size_t> transaction_2 = {2, 1, 3};
      my_tree.add_transaction(transaction_2, 1);
      // my_tree should be (root:0 (1:1 (0:1), 2:1 (1:1)))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 2);

      std::vector<size_t> transaction_3 = {2, 4, 0};
      my_tree.add_transaction(transaction_3, 2);
      // my_tree should be (root:0 (1:1 (0:1), 2:3 (1:1, 4:2 (0:2))))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 2);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 1);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[3], 1);

      std::vector<size_t> transaction_4 = {2, 4, 0};
      my_tree.add_transaction(transaction_4, 3);
      // my_tree should be (root:0 (1:1 (0:1), 2:6 (1:1, 4:5 (0:5))))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 2);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 0);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[3], 0);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[5], 1);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[6], 1);

    }
    void testTopKAddTransaction_2(void) {
      // Check that closed_node_count is only updated on min_length long nodes
      std::vector<size_t> ids = {2, 1, 4, 0, 5};
      std::vector<size_t> supports = {4, 3, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      size_t top_k = 3;
      size_t min_length = 3;

      fp_top_k_tree my_tree = fp_top_k_tree(header, top_k, min_length);


      std::vector<size_t> transaction_1 = {1, 0};
      my_tree.add_transaction(transaction_1, 1);
      // my_tree should be (root (1:1 (0:1)))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 0);

      std::vector<size_t> transaction_2 = {2, 1, 3};
      my_tree.add_transaction(transaction_2, 1);
      // my_tree should be (root:0 (1:1 (0:1), 2:1 (1:1)))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 0);

      std::vector<size_t> transaction_3 = {2, 4, 0};
      my_tree.add_transaction(transaction_3, 2);
      // my_tree should be (root:0 (1:1 (0:1), 2:3 (1:1, 4:2 (0:2))))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[1], 0);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 1);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[3], 0);

      std::vector<size_t> transaction_4 = {2, 4, 0, 5};
      my_tree.add_transaction(transaction_4, 3);
      // my_tree should be (root:0 (1:1 (0:1), 2:6 (1:1, 4:5 (0:5 (5:3)))))
      TS_ASSERT_EQUALS(my_tree.closed_node_count[2], 0);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[5], 1);
      TS_ASSERT_EQUALS(my_tree.closed_node_count[3], 1);

    }


    // test fp_top_k_tree::get_min_depth()
    void testTopKGetMinDepth(void){
      std::vector<size_t> ids = {2, 1, 4, 0, 5};
      std::vector<size_t> supports = {4, 3, 3, 2, 1};
      fp_tree_header header = fp_tree_header(ids, supports);
      size_t top_k = 3;
      size_t min_length = 3;

      fp_top_k_tree my_tree = fp_top_k_tree(header, top_k, min_length);

      TS_ASSERT_EQUALS(my_tree.get_min_depth(), 3);

      my_tree.root_prefix = std::vector<size_t>({2});
      TS_ASSERT_EQUALS(my_tree.get_min_depth(), 2);

      my_tree.root_prefix = std::vector<size_t>({2, 1});
      TS_ASSERT_EQUALS(my_tree.get_min_depth(), 1);

      my_tree.root_prefix = std::vector<size_t>({2, 1, 4});
      TS_ASSERT_EQUALS(my_tree.get_min_depth(), 1);

      my_tree.root_prefix = std::vector<size_t>({2, 1, 4, 0});
      TS_ASSERT_EQUALS(my_tree.get_min_depth(), 1);

    }


};


BOOST_FIXTURE_TEST_SUITE(_fp_tree_test, fp_tree_test)
BOOST_AUTO_TEST_CASE(testFPTreeDefaultConstructor) {
  fp_tree_test::testFPTreeDefaultConstructor();
}
BOOST_AUTO_TEST_CASE(testFPTreeConstructor) {
  fp_tree_test::testFPTreeConstructor();
}
BOOST_AUTO_TEST_CASE(testFPTreeCopyConstructor) {
  fp_tree_test::testFPTreeCopyConstructor();
}
BOOST_AUTO_TEST_CASE(testAddTransaction) {
  fp_tree_test::testAddTransaction();
}
BOOST_AUTO_TEST_CASE(testGetItemCounts) {
  fp_tree_test::testGetItemCounts();
}
BOOST_AUTO_TEST_CASE(testBuildHeader_1) {
  fp_tree_test::testBuildHeader_1();
}
BOOST_AUTO_TEST_CASE(testBuildHeader_2) {
  fp_tree_test::testBuildHeader_2();
}
BOOST_AUTO_TEST_CASE(testBuildHeader_3) {
  fp_tree_test::testBuildHeader_3();
}
BOOST_AUTO_TEST_CASE(testBuildHeader_4) {
  fp_tree_test::testBuildHeader_4();
}
BOOST_AUTO_TEST_CASE(testFlexToIdVector) {
  fp_tree_test::testFlexToIdVector();
}
BOOST_AUTO_TEST_CASE(testBuildTree_1) {
  fp_tree_test::testBuildTree_1();
}
BOOST_AUTO_TEST_CASE(testBuildTree_2) {
  fp_tree_test::testBuildTree_2();
}
BOOST_AUTO_TEST_CASE(testBuildTree_3) {
  fp_tree_test::testBuildTree_3();
}
BOOST_AUTO_TEST_CASE(testPruneTree1) {
  fp_tree_test::testPruneTree1();
}
BOOST_AUTO_TEST_CASE(testPruneTree2) {
  fp_tree_test::testPruneTree2();
}
BOOST_AUTO_TEST_CASE(testPruneTree3) {
  fp_tree_test::testPruneTree3();
}
BOOST_AUTO_TEST_CASE(testPruneTree4) {
  fp_tree_test::testPruneTree4();
}
BOOST_AUTO_TEST_CASE(testPruneTree5) {
  fp_tree_test::testPruneTree5();
}
BOOST_AUTO_TEST_CASE(testGetSupport) {
  fp_tree_test::testGetSupport();
}
BOOST_AUTO_TEST_CASE(testGetSupport2) {
  fp_tree_test::testGetSupport2();
}
BOOST_AUTO_TEST_CASE(testGetNumTransactions) {
  fp_tree_test::testGetNumTransactions();
}
BOOST_AUTO_TEST_CASE(testGetSupportsAtDepth) {
  fp_tree_test::testGetSupportsAtDepth();
}
BOOST_AUTO_TEST_CASE(testGetDescendantSupports) {
  fp_tree_test::testGetDescendantSupports();
}
BOOST_AUTO_TEST_CASE(testGetCondItemCounts) {
  fp_tree_test::testGetCondItemCounts();
}
BOOST_AUTO_TEST_CASE(testTopKGetMinSupportBound) {
  fp_tree_test::testTopKGetMinSupportBound();
}
BOOST_AUTO_TEST_CASE(testBuildTopKTree_1) {
  fp_tree_test::testBuildTopKTree_1();
}
BOOST_AUTO_TEST_CASE(testBuildTopKTree_2) {
  fp_tree_test::testBuildTopKTree_2();
}
BOOST_AUTO_TEST_CASE(testTopKAddTransaction_1) {
  fp_tree_test::testTopKAddTransaction_1();
}
BOOST_AUTO_TEST_CASE(testTopKAddTransaction_2) {
  fp_tree_test::testTopKAddTransaction_2();
}
BOOST_AUTO_TEST_CASE(testTopKGetMinDepth) {
  fp_tree_test::testTopKGetMinDepth();
}
BOOST_AUTO_TEST_SUITE_END()
