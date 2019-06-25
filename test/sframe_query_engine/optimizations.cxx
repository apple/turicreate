#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/util/aggregates.hpp>
#include <core/storage/query_engine/operators/operator_transformations.hpp>
#include <core/storage/sframe_data/sarray.hpp>

#define ENABLE_HISTORY_TRACKING_OPTIMIZATION true

using namespace turi;
using namespace turi::query_eval;


static const size_t n = 17;

/**  The array of planner nodes test different special cases of the
 *   optimization pipeline:
 *
 *   .v[0] -- reference: no-opt.
 *   .v[1] -- opt + naive
 *   .v[2] -- opt
 *   .v[3] -- opt, with many nodes in the history pre-materialized.
 *   .v[4] -- opt with zero length sframes to test this corner case.
 *   .v[5] -- opt with truncated sframes to test indexing and slicing, 0-n/2
 *   .v[6] -- opt with truncated sframes to test indexing and slicing, n/4-3*n/4
 *   .v[7] -- opt with truncated sframes to test indexing and slicing, n/2-n
 */

// Actually using arrays to make sure full + no-opt + no-opt-naive are all the same

struct node {
  using history_item = std::array<std::shared_ptr<planner_node>, 2>;
  std::array<std::shared_ptr<planner_node>, 8> v;
  std::set<history_item> history;
  
  void pull_history(const std::vector<node>& nv) {
    for(const node& n : nv) {
      history.insert(n.history.begin(), n.history.end());
      history.insert(history_item{{n.v[0], n.v[3]}});
    }
  }
}; 


////////////////////////////////////////////////////////////////////////////////
// General sources


static void add_sliced_info(node& ret, size_t m) {
  static std::map<pnode_ptr, pnode_ptr> sliced_graph_memo;
  ret.v[5] = query_eval::make_sliced_graph(ret.v[0], 0, m / 2, sliced_graph_memo);
  ret.v[6] = query_eval::make_sliced_graph(ret.v[0], m / 4, (3 * m) / 4, sliced_graph_memo);
  ret.v[7] = query_eval::make_sliced_graph(ret.v[0], m / 2, m, sliced_graph_memo);
};

static node source_sarray() {
  std::vector<flexible_type> data(n);
  
  for (size_t i = 0; i < n; ++i)
    data[i] = random::fast_uniform<size_t>(0,9);

  auto sa = std::make_shared<sarray<flexible_type>>();
  
  sa->open_for_write();
  turi::copy(data.begin(), data.end(), *sa);
  sa->close();

  node ret;
  for(auto& n : ret.v)
    n = op_sarray_source::make_planner_node(sa);

  {
    // Element 4 is zero length.

    auto sa = std::make_shared<sarray<flexible_type>>();

    sa->open_for_write();
    sa->close();

    ret.v[4] = op_sarray_source::make_planner_node(sa);
  }

  add_sliced_info(ret, n);

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});

  return ret;
}

static node empty_sarray() {

  auto sa = std::make_shared<sarray<flexible_type>>();

  sa->open_for_write();
  sa->close();

  node ret;
  for(auto& n : ret.v)
    n = op_sarray_source::make_planner_node(sa);

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});

  add_sliced_info(ret, 0);
  
  return ret;
}

static node zero_source_sarray() {

  std::vector<flexible_type> data(n);
  
  for (size_t i = 0; i < n; ++i)
    data[i] = 0;

  auto sa = std::make_shared<sarray<flexible_type>>();
  
  sa->open_for_write();
  turi::copy(data.begin(), data.end(), *sa);
  sa->close();

  node ret;
  for(auto& n : ret.v)
    n = op_sarray_source::make_planner_node(sa);

  {
    // Element 4 is zero length.

    auto sa = std::make_shared<sarray<flexible_type>>();

    sa->open_for_write();
    sa->close();

    ret.v[4] = op_sarray_source::make_planner_node(sa);
  }

  add_sliced_info(ret, data.size());

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});
  
  return ret;
}

static node binary_source_sarray() {
  std::vector<flexible_type> data(n);
  
  for (size_t i = 0; i < n; ++i) {
    data[i] = (i < 4) ? 1 : random::fast_uniform<int>(0,1);
  }

  auto sa = std::make_shared<sarray<flexible_type>>();
  
  sa->open_for_write();
  turi::copy(data.begin(), data.end(), *sa);
  sa->close();

  node ret;
  for(auto& n : ret.v)
    n = op_sarray_source::make_planner_node(sa);

  {
    // Element 4 is zero length.

    auto sa = std::make_shared<sarray<flexible_type>>();

    sa->open_for_write();
    sa->close();

    ret.v[4] = op_sarray_source::make_planner_node(sa);
  }

  add_sliced_info(ret, data.size());

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});
  
  return ret;
}


static node source_sframe(size_t n_columns) {

  std::vector<std::vector<flexible_type> > data(n_columns);
  
  for(size_t i = 0; i < n_columns; ++i) {
    data[i].resize(n);

    for(size_t j = 0; j < n; ++j) { 
      data[i][j] = random::fast_uniform<size_t>(0,9);
    }
  }

  std::vector<std::shared_ptr<sarray<flexible_type> > > sa_l(n_columns);

  for(size_t i = 0; i < n_columns; ++i) {
    
    auto sa = std::make_shared<sarray<flexible_type> >();
  
    sa->open_for_write();
    turi::copy(data[i].begin(), data[i].end(), *sa);
    sa->close();

    sa_l[i] = sa;
  }

  node ret;
  for(auto& n : ret.v)
    n = op_sframe_source::make_planner_node(sframe(sa_l));

  {
    for(size_t i = 0; i < n_columns; ++i) {

      auto sa = std::make_shared<sarray<flexible_type> >();

      sa->open_for_write();
      sa->close();

      sa_l[i] = sa;

    }
    ret.v[4] = op_sframe_source::make_planner_node(sframe(sa_l));
  }

  add_sliced_info(ret, data.size());

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});

  return ret;
}

static node shifted_source_sframe(size_t n_columns) {

  std::vector<std::vector<flexible_type> > data(n_columns);

  for(size_t i = 0; i < n_columns; ++i) {
    data[i].resize(2*n);

    for(size_t j = 0; j < 2*n; ++j) {
      data[i][j] = random::fast_uniform<size_t>(0,9);
    }
  }

  std::vector<std::shared_ptr<sarray<flexible_type> > > sa_l(n_columns);

  for(size_t i = 0; i < n_columns; ++i) {

    auto sa = std::make_shared<sarray<flexible_type> >();

    sa->open_for_write();
    turi::copy(data[i].begin(), data[i].end(), *sa);
    sa->close();

    sa_l[i] = sa;
  }

  node ret;
  for(auto& pn : ret.v)
    pn = op_sframe_source::make_planner_node(sframe(sa_l), n/2, n/2 + n);

  {
    for(size_t i = 0; i < n_columns; ++i) {

      auto sa = std::make_shared<sarray<flexible_type> >();

      sa->open_for_write();
      sa->close();

      sa_l[i] = sa;

    }
    ret.v[4] = op_sframe_source::make_planner_node(sframe(sa_l));
  }

  add_sliced_info(ret, n);

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});

  return ret;
}

static node empty_sframe(size_t n_columns) {

  std::vector<std::shared_ptr<sarray<flexible_type> > > sa_l(n_columns);

  for(size_t i = 0; i < n_columns; ++i) {

    auto sa = std::make_shared<sarray<flexible_type> >();

    sa->open_for_write();
    sa->close();

    sa_l[i] = sa;
  }

  node ret;
  for(auto& n : ret.v)
    n = op_sframe_source::make_planner_node(sframe(sa_l));

  add_sliced_info(ret, 0);

  ret.history.insert(node::history_item{{ret.v[0], ret.v[3]}});
  
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
// Transforms

static node make_union(node n1, node n2) {

  node ret; 

  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_union::make_planner_node(n1.v[i], n2.v[i]);

  ret.pull_history({n1, n2});
  
  return ret;
}

static node make_project(node n1, const std::vector<size_t>& indices) {

  node ret;

  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_project::make_planner_node(n1.v[i], indices);

  ret.pull_history({n1});
  
  return ret;
}

static node make_transform(node n1) {

  node ret;

  transform_type tr = [](const sframe_rows::row& r) -> flexible_type {
    flexible_type out = flex_int(1);

    for(size_t i = 0; i < r.size(); ++i) {
      out += r[i];
    }
    return out % 10; 
  };
  
  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_transform::make_planner_node(n1.v[i], tr, flex_type_enum::INTEGER);

  ret.pull_history({n1});
  
  return ret;
}

static node make_generalized_transform(node n1, size_t n_out) {

  std::vector<flex_type_enum> output_types(n_out, flex_type_enum::INTEGER);

  generalized_transform_type f = [=](const sframe_rows::row& r1, sframe_rows::row& r2) {

    size_t prod = 1;
    for(size_t i = 0; i < r1.size(); ++i)
      prod *= (1 + i + size_t(r1[i]));
    
    size_t src_idx = 0;
    for(size_t i = 0; i < n_out; ++i) {
      size_t idx_1 = (src_idx % r1.size()); ++src_idx;
      size_t idx_2 = (src_idx % r1.size()); ++src_idx;
          
      r2[i] = (prod + r1[idx_1] + r1[idx_2]) % 10;
    }
  };
      
  node ret;
  
  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_generalized_transform::make_planner_node(n1.v[i], f, output_types);

  ret.pull_history({n1});

  return ret;
}
  
static node make_logical_filter(node n1, node n2) {

  node ret;
  
  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_logical_filter::make_planner_node(n1.v[i], n2.v[i]);

  ret.pull_history({n1, n2});
  
  return ret;
}


static node make_append(node n1, node n2) {

  node ret;

  for(size_t i = 0; i < n1.v.size(); ++i)
    ret.v[i] = op_append::make_planner_node(n1.v[i], n2.v[i]);

  ret.pull_history({n1, n2});
  
  return ret;
}

static void check_sframes(sframe sf1, sframe sf2, std::string tag) {
  
  std::vector<std::vector<std::vector<flexible_type> > > results(2);
  
  sf1.get_reader()->read_rows(0, sf1.num_rows(), results[0]);
  sf2.get_reader()->read_rows(0, sf2.num_rows(), results[1]);
  
  bool all_okay = true; 

  for(size_t i = 1; i < results.size() && all_okay; ++i) {
    if(results[0].size() != results[i].size()) { all_okay = false; break; }
        
    for(size_t j = 0; j < results[0].size() && all_okay; ++j) {
      if(results[0][j].size() != results[i][j].size()) { all_okay = false; break; }
      
      for(size_t k = 0; k < results[0][j].size(); ++k) {
        if(results[0][j][k] != results[i][j][k]) { all_okay = false;  break; } // 
      }
    }
  }
  
  if(!all_okay) {

    logprogress_stream << "ERROR (left) NO-OPT != OPT (right) [run=" << tag << "]" << std::endl;

    logprogress_stream << "------------------PATTERN--------------------"  << std::endl;
    
    for(size_t j = 0; j < results[0].size(); ++j) {

      auto get_pattern = [&](size_t k, int i) -> char {
        if(results[0][j].size() <= k)
          return 'X';
        if(results[1][j].size() <= k)
          return 'X';

        int i1 = results[0][j][k];
        int i2 = results[1][j][k];

        if(i1 == i2)
          return ' ';

        if(i == 0) return '#';
        if(i < 3) return '.';
        if(i < 5) return '/';
        if(i < 8) return '\\';
        return 'O';
      };

      std::ostringstream ss;

      ss << "[ ";

      for(size_t k = 0; k < results[0][j].size(); ++k) {
        ss << get_pattern(k, results[0][j][k]) << " ";
      }

      ss << "] != [ ";
        
      for(size_t k = 0; k < results[1][j].size(); ++k) {
        ss << get_pattern(k, results[1][j][k]) << " ";
      }

      ss << "]";

      logprogress_stream << ss.str() << std::endl;
    }

    logprogress_stream << std::endl;
    logprogress_stream << "------------------ACTUAL--------------------"  << std::endl;
    for(size_t j = 0; j < results[0].size(); ++j) {
      std::ostringstream ss;

      ss << "[ ";

      for(size_t k = 0; k < results[0][j].size(); ++k) {
        ss << int(results[0][j][k]) << " ";
      }

      ss << "] != [ ";
        
      for(size_t k = 0; k < results[1][j].size(); ++k) {
        ss << int(results[1][j][k]) << " ";
      }

      ss << "]";

      logprogress_stream << ss.str() << std::endl;
    }

    logprogress_stream << "------------------REPORT--------------------"  << std::endl;

    std::vector<uint64_t> left_hashes(results[0][0].size(), 0);
    std::vector<uint64_t> right_hashes(results[1][0].size(), 0);
    
    for(size_t j = 0; j < results[0].size(); ++j) {
      for(size_t k = 0; k < results[0][j].size(); ++k) {
        left_hashes[k] = hash64(left_hashes[k], results[0][j][k]);
      }

      for(size_t k = 0; k < results[1][j].size(); ++k) {
        right_hashes[k] = hash64(right_hashes[k], results[1][j][k]);
      }
    }

    for(size_t i = 0; i < left_hashes.size(); ++i) {

      auto it = std::find(right_hashes.begin(), right_hashes.end(), left_hashes[i]);

      if(it == right_hashes.end()) {
        logprogress_stream << "Column " << i << ": not found in output.";
      } else {
        size_t idx = (it - right_hashes.begin());
        if(idx == i) {
          logprogress_stream << "Column " << i << ": correct " << std::endl;
        } else {
          logprogress_stream << "Column " << i << ": in position " << idx << std::endl;
        }
      }
    }
    
    ASSERT_TRUE(false); 
  }
}



#define _RUN(n) run(__LINE__, n)

static void run(size_t line, node n) {
  global_logger().set_log_level(LOG_INFO);
  materialize_options no_opt;
  no_opt.disable_optimization = true; 
  
  materialize_options naive;
  naive.naive_mode = true; 
  
  std::vector<sframe> out(4);
  
  logprogress_stream << std::endl;
  logprogress_stream << "################################################################" << std::endl;
  logprogress_stream << ">>> Prewarming Optimizations                                 <<<" << std::endl;
  logprogress_stream << ">>> " << line << std::endl;

  std::vector<decltype(n.history)::value_type> history_vect(n.history.begin(), n.history.end());

  random::shuffle(history_vect);

  for(size_t i = 0; i < std::min<size_t>(history_vect.size(), 10); ++i) {
    sframe sf_1 = planner().materialize(history_vect[i][0], no_opt);
    sframe sf_2 = planner().materialize(history_vect[i][1], materialize_options());

    check_sframes(sf_1, sf_2, "mixed-graph-materialize"); 
  }
   
  logprogress_stream << std::endl;
  logprogress_stream << "################################################################" << std::endl;
  logprogress_stream << ">>> Optimization Disabled                                    <<<" << std::endl;
  logprogress_stream << ">>> " << line << std::endl;

  out[0] = planner().materialize(n.v[0], no_opt);

  logprogress_stream << std::endl;
  logprogress_stream << "################################################################" << std::endl;
  logprogress_stream << ">>> Optimization Enabled, Naive Materialize                  <<<" << std::endl;
  logprogress_stream << ">>> " << line << std::endl;

  out[1] = planner().materialize(n.v[1], naive);

  logprogress_stream << std::endl;
  logprogress_stream << "################################################################" << std::endl;
  logprogress_stream << ">>> Optimization Enabled                                     <<<" << std::endl;
  logprogress_stream << ">>> " << line << std::endl;
  
  out[2] = planner().materialize(n.v[2], materialize_options());

  logprogress_stream << std::endl;
  logprogress_stream << "################################################################" << std::endl;
  logprogress_stream << ">>> Optimization Enabled, history of evaluation              <<<" << std::endl;
  logprogress_stream << ">>> " << line << std::endl;
  
  out[3] = planner().materialize(n.v[3], materialize_options());
  
  check_sframes(out[0], out[1], "naive");
  check_sframes(out[0], out[2], "Opt");
  check_sframes(out[0], out[3], "Opt-with-history");
}


struct opts  {
 public:
 
  void test_union_sarray() {
    node out = make_union(source_sarray(), source_sarray());
    _RUN(out);
  }

  void test_project_sframe() {
    node out = make_project(source_sframe(5), {0, 2, 4});
    _RUN(out);
  }

  void test_union_project_sframe() {
    random::seed(0);
    
    node n = source_sframe(5);
    
    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices;
      
      for(size_t i = 0; i < 5; ++i)
        indices.push_back(random::fast_uniform<size_t>(0, 9));

      if(i % 2 == 0)
        n = make_union(n, source_sframe(5));
      else 
        n = make_union(source_sframe(5), n);
                       
      n = make_project(n, indices);
    }
    
    _RUN(n);
  }

  void test_union_project_elimination_right() {

    node n1 = make_transform(source_sframe(2));
    node n2 = make_transform(source_sframe(2));
    
    node n = make_union(n1, n2);
    n = make_project(n, {0});

    _RUN(n);
  }

  void test_union_project_elimination_left() {

    node n1 = make_transform(source_sframe(2));
    node n2 = make_transform(source_sframe(2));
    
    node n = make_union(n1, n2);
    n = make_project(n, {1});

    _RUN(n);
  }
  

  void test_union_project_switch_places() {

    node n = source_sframe(2);
    node old_n = source_sframe(2);
    
    n = make_union(n, old_n);
    old_n = n;
    n = make_project(n, {3,0});

    n = make_union(n, old_n);
    
    _RUN(n);
  }
  
  void test_union_project_recursive_sframe_2() {
    random::seed(0);

    node n = source_sframe(5);
    node old_n = source_sframe(5);
    
    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices;

      for(size_t i = 0; i < 5; ++i)
        indices.push_back(random::fast_uniform<size_t>(0, 9));

      if(random::fast_uniform<size_t>(0, 1) == 0)
        n = make_union(n, old_n);
      else 
        n = make_union(old_n, n);
      
      old_n = n;
      
      n = make_project(n, indices);
    }
    
    _RUN(n);
  }
  
  void test_union_project_recursive_sframe_3() {
    random::seed(0);

    node n = source_sframe(5);
    node old_n = source_sframe(5);

    std::vector<node> nodes;
    
    for(size_t i = 0; i < 10; ++i) {

      std::vector<size_t> indices;
      
      for(size_t i = 0; i < 5; ++i)
        indices.push_back(random::fast_uniform<size_t>(0, 9));

      if(random::fast_uniform<size_t>(0, 1) == 0)
        n = make_union(n, old_n);
      else 
        n = make_union(old_n, n);

      nodes.push_back(n);
      
      old_n = n;
      
      n = make_project(n, indices);

      nodes.push_back(n);

      n = make_union(n, nodes[random::fast_uniform<size_t>(0, nodes.size() - 1)]);
      n = make_union(n, nodes[random::fast_uniform<size_t>(0, nodes.size() - 1)]);
    }
    
    _RUN(n);
  }

  void test_project_union_transform() {
    node n = source_sframe(5);
    node out = make_union(n, make_transform(make_project(n, {1, 2}) ) );
    _RUN(out); 
  }

  void test_eliminate_identity_projection_1() {
    node n = source_sframe(5);
    n = make_project(make_transform(n), {0});
    n = make_project(make_transform(n), {0});
    _RUN(n);
  }


  void test_eliminate_identity_projection_2() {
    node n1 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    node n2 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    
    node n = make_union(n1, n2);

    n = make_project(n, {1, 0, 3, 2});
    n = make_project(n, {3, 2, 1, 0});
    
    _RUN(n);
  }
  
  void test_eliminate_identity_projection_3() {
    random::seed(0);

    node n1 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    node n2 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    
    node n = make_union(n1, n2);

    std::vector<size_t> idx = {0, 1, 2, 3};
    
    for(size_t i = 0; i < 50; ++i) {
      random::shuffle(idx);
      n = make_project(n, idx);
    }
          
    _RUN(n);
  }


  void test_merge_projections() {
    node n1 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    node n2 = make_union(make_transform(source_sframe(5)),
                         make_transform(source_sframe(5)));
    
    node n3 = make_union(n1, n2);
    
    node n = make_project(n3, {0, 1, 2, 3});
    
    _RUN(n);
  }
  
  void test_project_union_transform_recursive_1() {
    random::seed(0);

    node n = source_sframe(5);

    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices = {random::fast_uniform<size_t>(0, 5 + i - 1)};
      n = make_union(n, make_transform(make_project(n, indices) ) );
    }

    _RUN(n);
  }

  void test_project_union_transform_recursive_2() {
    random::seed(0);

    node n = source_sframe(5);

    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices;
      
      for(size_t j = 0; j < 2; ++j)
        indices.push_back(random::fast_uniform<size_t>(0, 5 + i - 1));

      n = make_union(n, make_transform(make_project(n, indices) ) );
    }

    _RUN(n);
  }

  void test_project_union_transform_recursive_3() {
    random::seed(0);

    node n = source_sframe(5);

    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices;

      for(size_t j = 0; j < 4; ++j)
        indices.push_back(random::fast_uniform<size_t>(0, 5 + i - 1));

      n = make_union(n, make_transform(make_project(n, indices) ) );
    }

    _RUN(n);
  }

  void test_append_on_source() {
    node n = make_append(source_sframe(5), 
                         source_sframe(5));
    _RUN(n);
  }
  
  void test_project_append_exchange_1() {
    node n1 = source_sframe(5);
    node n2 = source_sframe(5);

    node n = make_project(make_append(n1, n2), {1, 3, 4});

    _RUN(n);
  }

  void test_project_append_exchange_2() {
    random::seed(0);

    node n = source_sframe(5);

    for(size_t i = 0; i < 20; ++i) {
      node n2 = source_sframe(5);

      std::vector<size_t> indices;
      for(size_t j = 0; j < 5; ++j)
        indices.push_back(random::fast_uniform<size_t>(0, 4));

      if(i %3 == 0)
        n = make_project(make_append(n, n2), indices);
      else
        n = make_project(make_append(n2, n), indices);
    }

    _RUN(n);
  }


  void test_project_logical_filter_exchange_1() {
    node n1 = source_sframe(5);
    node n2 = binary_source_sarray();

    node n = make_project(make_logical_filter(n1, n2), {1, 3});
    
    _RUN(n);
  }

  void test_project_logical_filter_exchange_2() {
    node n1 = source_sframe(5);
    node n2 = binary_source_sarray();

    node lf = make_logical_filter(n1, n2);
    
    node n = make_union(make_project(lf, {0,2,3}),
                        make_project(lf, {1,4}));

    _RUN(n);
  }
  /* 
   * TODO: These cases are currently impossible to produce
   * via the regular SFrame API since binary operations across of stuff of
   * unknown sizes will force materialization to check their size,
   * before allowing the plan to be created.
   * Thus there are no current means of generating such a plan except via
   * query optimization, but the query optimizations that reorder
   * the logical_filter have been disabled
   *
   * void disabled_test_project_logical_filter_exchange_3() {
   *   node n1 = source_sframe(5);
   *   node n2 = source_sframe(5);
   *   node mask = binary_source_sarray();

   *   node lf_1 = make_logical_filter(n1, mask);
   *   node lf_2 = make_logical_filter(n2, mask);

   *   node n = make_union(lf_1, lf_2);

   *   _RUN(n);
   * }


   * void disabled_test_project_logical_filter_exchange_4() {
   *   node n1 = source_sframe(5);
   *   node n2 = source_sframe(5);
   *   node mask = binary_source_sarray();

   *   node lf_1 = make_logical_filter(n1, mask);
   *   node lf_2 = make_logical_filter(n2, mask);

   *   node n = make_project(make_union(lf_1, lf_2), {1,0});

   *   _RUN(n);
   * }
   */
  void test_zero_logical_filter() {
    node n1 = source_sframe(5);
    node n2 = zero_source_sarray();

    node n = make_project(make_logical_filter(n1, n2), {1, 3});
    
    _RUN(n);
  }

  void test_union_filter_exchange_1() {
    node n1 = source_sframe(2);
    node n2 = source_sframe(2);

    node mask = binary_source_sarray();

    n1 = make_transform(n1);
    n1 = make_transform(n1);
    n2 = make_transform(n2);

    node n = make_logical_filter(make_union(n1, n2), mask);

    _RUN(n);
  }

  void test_union_filter_exchange_2() {
    random::seed(0);

    node n = source_sframe(5);

    for(size_t i = 0; i < 20; ++i) {

      std::vector<size_t> indices;

      for(size_t j = 0; j < 2; ++j)
        indices.push_back(random::fast_uniform<size_t>(0, 5 + i - 1));

      n = make_union(n, make_transform(make_project(n, indices) ) );
    }

    node mask = binary_source_sarray();

    n = make_logical_filter(n, mask);

    _RUN(n);
  }

  void test_empty_sframe() {
    node n = empty_sframe(5);
    _RUN(n);
  }

  void test_empty_append_sframe_collapse_1() {
    node n = empty_sframe(5);

    n = make_append(n, n);

    _RUN(n);
  }

  void test_empty_append_sframe_collapse_2() {
    node n = empty_sframe(5);

    for(size_t i = 0; i < 10; ++i) {
      n = make_append(n, n);
    }

    _RUN(n);
  }

  void test_empty_append_sframe_collapse_with_transform() {
    node n = empty_sframe(5);

    for(size_t i = 0; i < 5; ++i) {
      n = make_append(make_transform(n), make_transform(n));
    }

    _RUN(n);
  }

  void test_empty_append_sarray_collapse() {
    node n = empty_sarray();

    for(size_t i = 0; i < 10; ++i) {
      n = make_append(n, n);
    }

    _RUN(n);
  }
  
  void test_union_project_merge() {
    node n = source_sframe(5);
    n = make_generalized_transform(n, 5);

    n = make_union(make_project(n, {1, 2, 3}), make_project(n, {0, 4}));

    _RUN(n);
  }
  
  void test_union_project_merge_2() {
    node n = source_sframe(5);

    node n_src = make_generalized_transform(n, 10);
    
    n = n_src;
    
    for(size_t i = 0; i < 10; ++i)
      n = make_union(n, make_project(n_src, {i})); 

    _RUN(n);
  }

  void test_union_project_merge_2b() {
    
    node n_src = make_generalized_transform(source_sframe(10), 10);
    
    node n = n_src;
    
    for(size_t i = 0; i < 10; ++i)
      n = make_union(make_project(n, {0, 2, 1, 4, 3, 6, 5, 8, 7, 9}), make_project(n_src, {i}));
    
    _RUN(n);
  }
  
  void test_union_project_merge_3() {
    random::seed(0);

    node n = source_sframe(5);

    std::vector<node> node_list = {make_generalized_transform(n, 10)};

    for(size_t i = 0; i < 30; ++i) {
      size_t idx_1 = random::fast_uniform<size_t>(0, node_list.size() - 1);
      size_t proj_idx = random::fast_uniform<size_t>(0, 9);
      size_t idx_2 = random::fast_uniform<size_t>(0, node_list.size() - 1);

      node_list.push_back(make_union(node_list[idx_1], make_project(node_list[idx_2], {proj_idx})));
    }

    _RUN(node_list.back());
  }

  void test_union_project_merge_4() {
    
    node n_src = make_generalized_transform(source_sframe(5), 100);

    node n = make_union(make_project(n_src, {0}), make_project(n_src, {1}));
    
    for(size_t i = 2; i < 100; ++i) {
      n = make_union(n, make_project(n_src, {i}));
    } 

    _RUN(n);
  }

  void test_union_project_merge_5() {
    random::seed(0);

    node n = source_sframe(5);

    std::vector<node> node_list = {make_generalized_transform(n, 10)};

    for(size_t i = 0; i < 20; ++i) {
      size_t idx_1 = random::fast_uniform<size_t>(0, node_list.size() - 1);
      size_t idx_2 = random::fast_uniform<size_t>(0, node_list.size() - 1);
      size_t idx_3 = random::fast_uniform<size_t>(0, node_list.size() - 1);

      std::vector<size_t> project_indices(5);
      for(size_t& idx : project_indices) 
        idx = random::fast_uniform<size_t>(0, 9);

      node_list.push_back(make_union(node_list[idx_1], make_project(node_list[idx_2], project_indices)));
      node_list.push_back(make_union(node_list[idx_1], make_generalized_transform(node_list[idx_3], 10)));
    }

    n = make_union(node_list[0], node_list[1]);
    
    for(size_t i = 2; i < node_list.size(); ++i) {
      size_t idx = random::fast_uniform<size_t>(0, 14);
      n = make_union(n, make_project(node_list[i], {idx}));
    }
    
    _RUN(n);
  }

  void test_union_shifted_sframes() {
    node n1 = source_sframe(5);
    node n2 = shifted_source_sframe(5);

    node n = make_union(n1, n2);

    _RUN(n);
  }

  void test_union_duplication() {
    node n = source_sframe(1);

    n = make_union(n, n);

    _RUN(n);
  }

  void test_union_duplication_2() {
    node n = source_sframe(1);

    n = make_union(n, n);

    _RUN(n);
  }

  void test_union_duplication_3() {
    node n = source_sframe(1);

    for(size_t i = 0; i < 5; ++i) {
      n = make_union(n, n);
    }

    _RUN(n);
  }

  void test_regression_union_project_identity_issue() {

    node n = make_generalized_transform(source_sframe(5), 2);

    n = make_union(n, make_project(n, {0,1}));

    _RUN(n);
  }

  void test_source_merging_as_sarrays() {
    random::seed(0);

    node base_1 = make_union(source_sframe(5), shifted_source_sframe(5));

    node n = base_1;

    for(size_t i = 0; i < 20; ++i) {
      size_t idx_1 = random::fast_uniform<size_t>(0, 9);

      n = make_union(n, make_transform(make_project(base_1, {idx_1})));
    }

    _RUN(n);
  }

  void test_source_merging_as_sframes() {
    random::seed(0);

    node base_1 = make_union(source_sframe(5), shifted_source_sframe(5));

    node n = base_1;

    for(size_t i = 0; i < 20; ++i) {
      size_t idx_1 = random::fast_uniform<size_t>(0, 9);
      size_t idx_2 = random::fast_uniform<size_t>(0, 9);

      n = make_union(n, make_transform(make_project(base_1, {idx_1, idx_2})));
    }

    _RUN(n);
  }

  
};

BOOST_FIXTURE_TEST_SUITE(_opts, opts)
BOOST_AUTO_TEST_CASE(test_union_sarray) {
  opts::test_union_sarray();
}
BOOST_AUTO_TEST_CASE(test_project_sframe) {
  opts::test_project_sframe();
}
BOOST_AUTO_TEST_CASE(test_union_project_sframe) {
  opts::test_union_project_sframe();
}
BOOST_AUTO_TEST_CASE(test_union_project_elimination_right) {
  opts::test_union_project_elimination_right();
}
BOOST_AUTO_TEST_CASE(test_union_project_elimination_left) {
  opts::test_union_project_elimination_left();
}
BOOST_AUTO_TEST_CASE(test_union_project_switch_places) {
  opts::test_union_project_switch_places();
}
BOOST_AUTO_TEST_CASE(test_union_project_recursive_sframe_2) {
  opts::test_union_project_recursive_sframe_2();
}
BOOST_AUTO_TEST_CASE(test_union_project_recursive_sframe_3) {
  opts::test_union_project_recursive_sframe_3();
}
BOOST_AUTO_TEST_CASE(test_project_union_transform) {
  opts::test_project_union_transform();
}
BOOST_AUTO_TEST_CASE(test_eliminate_identity_projection_1) {
  opts::test_eliminate_identity_projection_1();
}
BOOST_AUTO_TEST_CASE(test_eliminate_identity_projection_2) {
  opts::test_eliminate_identity_projection_2();
}
BOOST_AUTO_TEST_CASE(test_eliminate_identity_projection_3) {
  opts::test_eliminate_identity_projection_3();
}
BOOST_AUTO_TEST_CASE(test_merge_projections) {
  opts::test_merge_projections();
}
BOOST_AUTO_TEST_CASE(test_project_union_transform_recursive_1) {
  opts::test_project_union_transform_recursive_1();
}
BOOST_AUTO_TEST_CASE(test_project_union_transform_recursive_2) {
  opts::test_project_union_transform_recursive_2();
}
BOOST_AUTO_TEST_CASE(test_project_union_transform_recursive_3) {
  opts::test_project_union_transform_recursive_3();
}
BOOST_AUTO_TEST_CASE(test_append_on_source) {
  opts::test_append_on_source();
}
BOOST_AUTO_TEST_CASE(test_project_append_exchange_1) {
  opts::test_project_append_exchange_1();
}
BOOST_AUTO_TEST_CASE(test_project_append_exchange_2) {
  opts::test_project_append_exchange_2();
}
BOOST_AUTO_TEST_CASE(test_project_logical_filter_exchange_1) {
  opts::test_project_logical_filter_exchange_1();
}
BOOST_AUTO_TEST_CASE(test_project_logical_filter_exchange_2) {
  opts::test_project_logical_filter_exchange_2();
}
BOOST_AUTO_TEST_CASE(test_zero_logical_filter) {
  opts::test_zero_logical_filter();
}
BOOST_AUTO_TEST_CASE(test_union_filter_exchange_1) {
  opts::test_union_filter_exchange_1();
}
BOOST_AUTO_TEST_CASE(test_union_filter_exchange_2) {
  opts::test_union_filter_exchange_2();
}
BOOST_AUTO_TEST_CASE(test_empty_sframe) {
  opts::test_empty_sframe();
}
BOOST_AUTO_TEST_CASE(test_empty_append_sframe_collapse_1) {
  opts::test_empty_append_sframe_collapse_1();
}
BOOST_AUTO_TEST_CASE(test_empty_append_sframe_collapse_2) {
  opts::test_empty_append_sframe_collapse_2();
}
BOOST_AUTO_TEST_CASE(test_empty_append_sframe_collapse_with_transform) {
  opts::test_empty_append_sframe_collapse_with_transform();
}
BOOST_AUTO_TEST_CASE(test_empty_append_sarray_collapse) {
  opts::test_empty_append_sarray_collapse();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge) {
  opts::test_union_project_merge();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge_2) {
  opts::test_union_project_merge_2();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge_2b) {
  opts::test_union_project_merge_2b();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge_3) {
  opts::test_union_project_merge_3();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge_4) {
  opts::test_union_project_merge_4();
}
BOOST_AUTO_TEST_CASE(test_union_project_merge_5) {
  opts::test_union_project_merge_5();
}
BOOST_AUTO_TEST_CASE(test_union_shifted_sframes) {
  opts::test_union_shifted_sframes();
}
BOOST_AUTO_TEST_CASE(test_union_duplication) {
  opts::test_union_duplication();
}
BOOST_AUTO_TEST_CASE(test_union_duplication_2) {
  opts::test_union_duplication_2();
}
BOOST_AUTO_TEST_CASE(test_union_duplication_3) {
  opts::test_union_duplication_3();
}
BOOST_AUTO_TEST_CASE(test_regression_union_project_identity_issue) {
  opts::test_regression_union_project_identity_issue();
}
BOOST_AUTO_TEST_CASE(test_source_merging_as_sarrays) {
  opts::test_source_merging_as_sarrays();
}
BOOST_AUTO_TEST_CASE(test_source_merging_as_sframes) {
  opts::test_source_merging_as_sframes();
}
BOOST_AUTO_TEST_SUITE_END()
