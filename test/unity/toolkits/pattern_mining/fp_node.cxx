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
#include <toolkits/pattern_mining/fp_node.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::pattern_mining;

/**
 *  Run tests.
*/
struct fp_node_test  {

  public:
    void testFPNodeConstruction(void) {
      // Test fp_node Construction
      std::shared_ptr<fp_node> my_node = std::make_shared<fp_node>(0);
      TS_ASSERT(my_node->item_id == 0);              // Id is 0
      TS_ASSERT(my_node->item_count == 0);           // Count is 0
      TS_ASSERT(!(my_node->next_node));                // Null Pointer
      TS_ASSERT(my_node->children_nodes.size() == 0); // No Children
    }

    void testFPNodeAddChild(void) {
      // Test adding an fp_node
      std::shared_ptr<fp_node> root_node = std::make_shared<fp_node>(ROOT_ID);
      fp_node* second_node = root_node->add_child(1);

      // Check Posize_ters
      TS_ASSERT(root_node->children_nodes.size() == 1);
      TS_ASSERT(root_node->children_nodes[0].get() == second_node);
      TS_ASSERT(second_node->parent_node == root_node.get());
    }

    void testFPNodeGetChild(void) {
       // Test getting children from an fp_node
      std::shared_ptr<fp_node> root_node = std::make_shared<fp_node>(ROOT_ID);
      fp_node* second_node = root_node->add_child(0);
      fp_node* third_node = root_node->add_child(2);

      fp_node* child_node = root_node->get_child(0);

      // Check Posize_ters
      TS_ASSERT(root_node->children_nodes.size() == 2);
      TS_ASSERT(second_node->parent_node == root_node.get());
      TS_ASSERT(child_node);
      TS_ASSERT(child_node->parent_node == root_node.get());
      TS_ASSERT(third_node->parent_node == root_node.get());

      TS_ASSERT(child_node == second_node);
    }

    void testFPNodeGetPathToRoot(void){
      // test get_path_to_root()

      // build tree
      std::shared_ptr<fp_node> root_node = std::make_shared<fp_node>(ROOT_ID);
      root_node->add_child(0);
      root_node->add_child(1);
      root_node->add_child(2);
      root_node->children_nodes[0]->add_child(3);
      root_node->children_nodes[0]->add_child(4);
      root_node->children_nodes[0]->children_nodes[0]->add_child(5);

      std::vector<size_t> path;
      std::vector<size_t> expected;

      path = root_node->get_path_to_root();
      expected = {};
      TS_ASSERT_EQUALS(path, expected);

       path = root_node->children_nodes[0]->children_nodes[0]-> \
             children_nodes[0]->get_path_to_root();
      expected = {5,3,0};
      TS_ASSERT_EQUALS(path, expected);

      path = root_node->children_nodes[0]->children_nodes[1]->get_path_to_root();
      expected = {4,0};
      TS_ASSERT_EQUALS(path, expected);

      path = root_node->children_nodes[2]->get_path_to_root();
      expected = {2};
      TS_ASSERT_EQUALS(path, expected);

      path = root_node->get_path_to_root();
      expected = {};
      TS_ASSERT_EQUALS(path, expected);
    }

    void testFPNodeIsClosed(void) {
      // build tree
      std::shared_ptr<fp_node> root_node = std::make_shared<fp_node>(ROOT_ID);
      auto child_node0 = root_node->add_child(0);
      child_node0->item_count = 4;
      auto child_node1 = root_node->add_child(1);
      child_node1->item_count = 2;
      auto child_node2 = root_node->add_child(2);
      child_node2->item_count = 1;
      auto child_node3 = child_node0->add_child(3);
      child_node3->item_count = 2;
      auto child_node4 = child_node0->add_child(4);
      child_node4->item_count = 2;
      auto child_node5 = child_node3->add_child(5);
      child_node5->item_count = 2;

      TS_ASSERT_EQUALS(child_node0->is_closed(), true);
      TS_ASSERT_EQUALS(child_node1->is_closed(), true);
      TS_ASSERT_EQUALS(child_node2->is_closed(), true);
      TS_ASSERT_EQUALS(child_node3->is_closed(), false);
      TS_ASSERT_EQUALS(child_node4->is_closed(), true);
      TS_ASSERT_EQUALS(child_node5->is_closed(), true);

    }

    void testFPNodeErase(void) {
      // build tree
      std::shared_ptr<fp_node> root_node = std::make_shared<fp_node>(ROOT_ID);
      fp_node* child_node0 = root_node->add_child(0);
      root_node->add_child(1);
      root_node->add_child(2);
      fp_node* child_node3 = child_node0->add_child(3);
      fp_node* child_node4 = child_node0->add_child(4);
      child_node3->add_child(5);

      // Tree should be (root (0:0 (3:0 (5:0), 4:0), 1:0, 2:0))

      TS_ASSERT_EQUALS(root_node.use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0].use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[1].use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[2].use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->\
          children_nodes[0].use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->\
          children_nodes[1].use_count(), 1);
      TS_ASSERT_EQUALS(root_node->children_nodes[0]->\
          children_nodes[0]->children_nodes[0].use_count(), 1);

      std::weak_ptr<fp_node> weak_child_node4 = child_node0->children_nodes[1];
      child_node4->erase();
      TS_ASSERT_EQUALS(child_node0->children_nodes.size(), 1);
      TS_ASSERT(weak_child_node4.expired());

      std::weak_ptr<fp_node> weak_child_node3 = child_node0->children_nodes[0];
      std::weak_ptr<fp_node> weak_child_node5 = child_node3->children_nodes[0];
      child_node3->erase();
      TS_ASSERT_EQUALS(child_node0->children_nodes.size(), 0);
      TS_ASSERT(weak_child_node3.expired());
      TS_ASSERT(weak_child_node5.expired());

    }


};


BOOST_FIXTURE_TEST_SUITE(_fp_node_test, fp_node_test)
BOOST_AUTO_TEST_CASE(testFPNodeConstruction) {
  fp_node_test::testFPNodeConstruction();
}
BOOST_AUTO_TEST_CASE(testFPNodeAddChild) {
  fp_node_test::testFPNodeAddChild();
}
BOOST_AUTO_TEST_CASE(testFPNodeGetChild) {
  fp_node_test::testFPNodeGetChild();
}
BOOST_AUTO_TEST_CASE(testFPNodeGetPathToRoot) {
  fp_node_test::testFPNodeGetPathToRoot();
}
BOOST_AUTO_TEST_CASE(testFPNodeIsClosed) {
  fp_node_test::testFPNodeIsClosed();
}
BOOST_AUTO_TEST_CASE(testFPNodeErase) {
  fp_node_test::testFPNodeErase();
}
BOOST_AUTO_TEST_SUITE_END()
