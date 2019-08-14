#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <unity/lib/unity_graph.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/server/toolkits/recsys/data.hpp>
#include <unity/server/toolkits/recsys/schema_entry.hpp>
#include <unity/server/toolkits/recsys/data_view.hpp>
#include <unity/server/toolkits/recsys/util.hpp>

using namespace turi;
using namespace turi::recsys;

turi::distributed_control dc;

struct recsys_util_test {

  recsys_data rec_data;

 public:
  recsys_util_test() {
    std::cout << "Initialize recsys data" << std::endl;
    std::vector<schema_entry> schema = {
      schema_entry("user", schema_entry::CATEGORICAL, flex_type_enum::STRING),
      schema_entry("item", schema_entry::CATEGORICAL, flex_type_enum::STRING)
      // schema_entry("rating", schema_entry::REAL)
    };

    // 2 users , 6 items.
    std::vector< std::vector<flexible_type> > raw_data = {
      {1,1},
      {1,2},
      {1,3},
      {2,4},
      {2,5},
      {2,6}};

    // User 1 always rates 1, and user 2 always 2.
    std::vector<double> response{1,1,1,2,2,2};
    rec_data.set_primary_schema(schema);
    rec_data.set_primary_observations(raw_data, response);

    rec_data.finish();
  }

  void test_create_user_item_graph() {

    TS_ASSERT_EQUALS(rec_data.size(), 6);
    TS_ASSERT(!rec_data.empty());

    const data_view& view = rec_data.get_full_view();

    unity_graph* graph = create_user_item_rating_graph_from_data_view(view, "user", "item");

    const size_t middle_index = (size_t(1) << (8*sizeof(size_t) - 2));

    // Map from user/item id to the global index in the recsys data;
    size_t user_idx = view.column_index("user");
    size_t item_idx = view.column_index("item");
    std::unordered_map<flexible_type, size_t> uid_to_global_index;
    std::unordered_map<flexible_type, size_t> vid_to_global_index;
    for (auto obs : view) {
      auto user_entry = obs.at(user_idx);
      auto item_entry = obs.at(item_idx);
      uid_to_global_index[user_entry.feature_value()] = user_entry.feature_index();
      vid_to_global_index[item_entry.feature_value()] = item_entry.feature_index() + middle_index;
    }

    // Check size
    auto summary = graph->summary();
    TS_ASSERT_EQUALS((int)summary["num_vertices"], 8);
    TS_ASSERT_EQUALS((int)summary["num_edges"], 6);

    // Check fields
    std::vector<std::string> fields_vector = graph->get_fields();
    std::unordered_set<std::string> fields(fields_vector.begin(), fields_vector.end());
    TS_ASSERT_EQUALS(fields, (std::unordered_set<std::string>{"__id", "__src_id", "__dst_id", "response"}));

    // Check vertices
    std::vector<flexible_type> empty_vec;
    options_map_t empty_map;
    dataframe_t vertices = graph->get_vertices(empty_vec, empty_map)->head(size_t(-1));
    TS_ASSERT_EQUALS(vertices.nrows(), 8);
    TS_ASSERT_EQUALS(vertices.ncols(), 1);

    std::unordered_set<flexible_type> vertex_ids((vertices.values["__id"]).begin(), (vertices.values["__id"]).end());

    std::unordered_set<flexible_type> expected_vertex_ids;
    for (auto& kv : uid_to_global_index) {
      expected_vertex_ids.insert(kv.second);
    }
    for (auto& kv : vid_to_global_index) {
      expected_vertex_ids.insert(kv.second);
    }
    TS_ASSERT_EQUALS(vertex_ids, expected_vertex_ids);

    // Check edges
    dataframe_t edges = graph->get_edges(empty_vec, empty_vec, empty_map)->head(size_t(-1));
    TS_ASSERT_EQUALS(edges.nrows(), 6);
    TS_ASSERT_EQUALS(edges.ncols(), 3);

    size_t user1_count = 0;
    size_t user2_count = 0;
    for (size_t i = 0; i < edges.nrows(); ++i) {
      if (edges.values["__src_id"][i] == uid_to_global_index[1]) {
        TS_ASSERT_EQUALS(edges.values["response"][i], flex_float(1));
        ++user1_count;
      }
      if (edges.values["__src_id"][i] == uid_to_global_index[2]) {
        TS_ASSERT_EQUALS(edges.values["response"][i], flex_float(2));
        ++user2_count;
      }
    }
    TS_ASSERT_EQUALS(user1_count, 3);
    TS_ASSERT_EQUALS(user2_count, 3);
  }
};

BOOST_FIXTURE_TEST_SUITE(_recsys_util_test, recsys_util_test)
BOOST_AUTO_TEST_CASE(test_create_user_item_graph) {
  recsys_util_test::test_create_user_item_graph();
}
BOOST_AUTO_TEST_SUITE_END()
