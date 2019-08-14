#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <boost/bind.hpp>

using namespace turi;

struct column {
  std::string name;
  flex_type_enum type;
  std::vector<flexible_type> data;
};

struct sgraph_compute_test  {
 public:

  sframe create_sframe(const std::vector<column>& columns) {
    sframe ret;
    ret.open_for_write({}, {});
    ret.close();
    for (auto& col : columns) {
      std::shared_ptr<sarray<flexible_type>> sa = std::make_shared<sarray<flexible_type>>();
      sa->open_for_write();
      sa->set_type(col.type);
      turi::copy(col.data.begin(), col.data.end(), *sa);
      sa->close();
      ret = ret.add_column(sa, col.name);
    }
    return ret;
  }

  sgraph create_ring_graph(size_t nverts, size_t npartition, bool bidirection) {
    sgraph g(npartition);
    std::vector<flexible_type> sources;
    std::vector<flexible_type> targets;
    std::vector<flexible_type> ids;
    std::vector<flexible_type> data;
    for (size_t i = 0; i < nverts; ++i) {
      sources.push_back(i);
      targets.push_back((i + 1) % nverts);;
      data.push_back(1.0);
      ids.push_back(i);
    }
    column source_col = {
      "source",
      flex_type_enum::INTEGER,
      sources
    };
    column target_col = {
      "target",
      flex_type_enum::INTEGER,
      targets
    };
    column data_col = {
      "data",
      flex_type_enum::FLOAT,
      data
    };
    column id_col = {
      "id",
      flex_type_enum::INTEGER,
      ids
    };
    sframe edge_data = create_sframe({source_col, target_col, data_col});

    sframe vertex_data = create_sframe({id_col, data_col});

    // Add one direction
    g.add_edges(edge_data, "source", "target");
    if (bidirection) {
      // Add the other direction
      g.add_edges(edge_data, "target", "source");
    }
    // Add vertex data
    g.add_vertices(vertex_data, "id");

    return g;
  }

  sgraph create_star_graph(size_t nverts, size_t npartition) {
    sgraph g(npartition);
    std::vector<flexible_type> sources;
    std::vector<flexible_type> targets;
    std::vector<flexible_type> ids;
    std::vector<flexible_type> data;
    for (size_t i = 0; i < nverts; ++i) {
      if (i > 0) {
        sources.push_back(i);
        targets.push_back(0);;
      }
      data.push_back(1.0);
      ids.push_back(i);
    }
    column source_col = {
      "source",
      flex_type_enum::INTEGER,
      sources
    };
    column target_col = {
      "target",
      flex_type_enum::INTEGER,
      targets
    };
    column data_col = {
      "data",
      flex_type_enum::FLOAT,
      data
    };
    column id_col = {
      "id",
      flex_type_enum::INTEGER,
      ids
    };
    sframe edge_data = create_sframe({source_col, target_col});
    sframe vertex_data = create_sframe({id_col, data_col});
    // Add one direction
    g.add_edges(edge_data, "source", "target");
    // // Add vertex data
    g.add_vertices(vertex_data, "id");
    return g;
  }


  void test_triple_apply_edge_data_modification() {
    // Create an edge field, and assign it the value of the sum of source and target ids.
    size_t n_vertex = 1000;
    size_t n_partition = 4;
    sgraph g = create_ring_graph(n_vertex, n_partition, false /* one direction */);

    for (size_t i = 0; i < 2; ++i) {
      g.init_edge_field("id_sum", flex_int(0));
      size_t field_id = g.get_edge_field_id("id_sum");
      TS_ASSERT_EQUALS(field_id, 3);
      if (i == 0) {
        // test regular triple_apply
        sgraph_compute::triple_apply(g,
                                     [=](sgraph_compute::edge_scope& scope) {
                                       scope.edge()[field_id] = scope.source()[0] + scope.target()[0];
                                     },
                                     {}, {"id_sum"});
      } else if (i == 1) {
        // test batch triple_apply
        sgraph_compute::batch_triple_apply_mock(g,
                                                [=](sgraph_compute::edge_scope& scope) {
                                                  scope.edge()[field_id] = scope.source()[0] + scope.target()[0];
                                                },
                                                {}, {"id_sum"});
      }
      sframe edge_sframe = g.get_edges();
      std::vector<std::vector<flexible_type>> edge_data_rows;
      edge_sframe.get_reader()->read_rows(0, edge_sframe.size(), edge_data_rows);
      for (auto& row : edge_data_rows) {
        TS_ASSERT_EQUALS(int(row[0] + row[1]), int(row[3]));
      }
      g.remove_edge_field("id_sum");
    }
  }


  std::vector<std::pair<flexible_type, flexible_type>> mr_degree_count(sgraph& g, sgraph::edge_direction dir) {
    sgraph_compute::sgraph_engine<flexible_type> ga;
    typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
    typedef sgraph::edge_direction edge_direction;
    std::vector<std::shared_ptr<sarray<flexible_type>>> gather_results = ga.gather(g,
                                                        [](const graph_data_type& center, 
                                                           const graph_data_type& edge, 
                                                           const graph_data_type& other, 
                                                           edge_direction edgedir,
                                                           flexible_type& combiner) {
                                                          combiner = combiner + 1;
                                                        },
                                                        flexible_type(0),
                                                        dir);
    std::vector<std::shared_ptr<sarray<flexible_type>>> vertex_ids = g.fetch_vertex_data_field(sgraph::VID_COLUMN_NAME);

    TS_ASSERT_EQUALS(gather_results.size(), vertex_ids.size());
    std::vector<std::pair<flexible_type, flexible_type>> ret;

    for (size_t i = 0;i < gather_results.size(); ++i) {
      std::vector<flexible_type> degree_vec;
      std::vector<flexible_type> id_vec;
      gather_results[i]->get_reader()->read_rows(0, g.num_vertices(), degree_vec);
      vertex_ids[i]->get_reader()->read_rows(0, g.num_vertices(), id_vec);
      TS_ASSERT_EQUALS(degree_vec.size(), id_vec.size());
      for (size_t j = 0; j < degree_vec.size(); ++j) {
        ret.push_back({id_vec[j], degree_vec[j]});
      }
    }
    return ret;
  }


  std::vector<std::pair<flexible_type, flexible_type>> triple_apply_degree_count(
    sgraph& g,
    sgraph::edge_direction dir,
    bool use_batch_triple_apply_mock = false) {

    sgraph_compute::triple_apply_fn_type fn;
    g.init_vertex_field("__degree__", flex_int(0));
    std::vector<std::string> vertex_fields = g.get_vertex_fields();
    size_t degree_idx;
    for (size_t i = 0; i < vertex_fields.size(); ++i) {
      if (vertex_fields[i] == "__degree__") degree_idx = i;
    }

    if (dir == sgraph::edge_direction::IN_EDGE) {
      fn = [=](sgraph_compute::edge_scope& scope) {
        scope.lock_vertices();
        scope.target()[degree_idx]++;
        scope.unlock_vertices();
      };
    } else if (dir == sgraph::edge_direction::OUT_EDGE) {
      fn = [=](sgraph_compute::edge_scope& scope) {
        scope.lock_vertices();
        scope.source()[degree_idx]++;
        scope.unlock_vertices();
      };
    } else {
      fn = [=](sgraph_compute::edge_scope& scope) {
        scope.lock_vertices();
        scope.source()[degree_idx]++;
        scope.target()[degree_idx]++;
        scope.unlock_vertices();
      };
    }

    if (use_batch_triple_apply_mock) {
      sgraph_compute::triple_apply(g, fn, {"__degree__"});
    } else {
      sgraph_compute::batch_triple_apply_mock(g, fn, {"__degree__"});
    }

    auto result = g.fetch_vertex_data_field("__degree__");
    auto vertex_ids = g.fetch_vertex_data_field(sgraph::VID_COLUMN_NAME);
    std::vector<std::pair<flexible_type, flexible_type>> ret;
    for (size_t i = 0; i < result.size(); ++i) {
      std::vector<flexible_type> degree_vec;
      std::vector<flexible_type> id_vec;
      result[i]->get_reader()->read_rows(0, g.num_vertices(), degree_vec);
      vertex_ids[i]->get_reader()->read_rows(0, g.num_vertices(), id_vec);
      for (size_t j = 0; j < degree_vec.size(); ++j) {
        ret.push_back({id_vec[j], degree_vec[j]});
      }
    }
    g.remove_vertex_field("__degree__");
    return ret;
  }

  void test_basic_edge_count() {
    size_t n_vertex = 1000;
    size_t n_partition = 4;

    typedef sgraph::edge_direction edge_direction;
    typedef std::function<std::vector<std::pair<flexible_type, flexible_type>>(
        sgraph& g, sgraph::edge_direction dir)> degree_count_fn_type;

    std::vector<degree_count_fn_type>
      degree_count_functions{
        boost::bind(&sgraph_compute_test::mr_degree_count, this, _1, _2),
        boost::bind(&sgraph_compute_test::triple_apply_degree_count, this, _1, _2, false), // triple_apply_simple
        boost::bind(&sgraph_compute_test::triple_apply_degree_count, this, _1, _2, true) // triple_apply_batch
      };

    for (auto degree_count_fn: degree_count_functions) {
      {
        // for single directional ring graph
        sgraph g = create_ring_graph(n_vertex, n_partition, false /* one direction */);
        std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
        TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
        for (size_t i = 0; i < g.num_vertices(); ++i) {
          TS_ASSERT_EQUALS((int)out_degree[i].second, 1);
          TS_ASSERT_EQUALS((int)in_degree[i].second, 1);
          TS_ASSERT_EQUALS((int)total_degree[i].second, 2);
          TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
        }
      }

      {
        // for bi-directional ring graph
        sgraph g = create_ring_graph(n_vertex, n_partition, true /* bi direction */);
        std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
        TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
        for (size_t i = 0; i < g.num_vertices(); ++i) {
          TS_ASSERT_EQUALS((int)out_degree[i].second, 2);
          TS_ASSERT_EQUALS((int)in_degree[i].second, 2);
          TS_ASSERT_EQUALS((int)total_degree[i].second, 4);
          TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
        }
      }
      {
        // for star graph
        sgraph g = create_star_graph(n_vertex, n_partition);
        std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
        std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
        TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
        TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
        for (size_t i = 0; i < g.num_vertices(); ++i) {
          TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
          if (in_degree[i].first == 0) {
            TS_ASSERT_EQUALS((int)in_degree[i].second, n_vertex -1);
          } else {
            TS_ASSERT_EQUALS((int)in_degree[i].second, 0);
          }
          if (out_degree[i].first == 0) {
            TS_ASSERT_EQUALS((int)out_degree[i].second, 0);
          } else {
            TS_ASSERT_EQUALS((int)out_degree[i].second, 1);
          }
          if (total_degree[i].first == 0) {
            TS_ASSERT_EQUALS((int)total_degree[i].second, n_vertex -1);
          } else {
            TS_ASSERT_EQUALS((int)total_degree[i].second, 1);
          }
        }
      }
    }
  }

  void compute_pagerank(sgraph& g, size_t num_iter) {
    sgraph_compute::sgraph_engine<flexible_type> ga;
    typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
    typedef sgraph::edge_direction edge_direction;
    // count the outgoing degree
    std::vector<std::shared_ptr<sarray<flexible_type>>> ret = ga.gather(g,
                                                        [](const graph_data_type& center, 
                                                           const graph_data_type& edge, 
                                                           const graph_data_type& other, 
                                                           edge_direction edgedir,
                                                           flexible_type& combiner) {
                                                        combiner = combiner + 1;
                                                        },
                                                        flexible_type(0),
                                                        edge_direction::OUT_EDGE);
    // merge the outgoing degree to graph
    std::vector<sframe>& vdata = g.vertex_group();
    for (size_t i = 0; i < g.get_num_partitions(); ++i) {
      ASSERT_LT(i, vdata.size());
      ASSERT_LT(i, ret.size());
      vdata[i] = vdata[i].add_column(ret[i], "__out_degree__");
    }

    size_t degree_idx = vdata[0].column_index("__out_degree__");
    size_t data_idx = vdata[0].column_index("data");

    // now we compute the pagerank
    for (size_t iter = 0; iter < num_iter; ++iter) {
      ret = ga.gather(g,
          [=](const graph_data_type& center,
              const graph_data_type& edge,
              const graph_data_type& other,
              edge_direction edgedir,
              flexible_type& combiner) {
             combiner = combiner + 0.85 * (other[data_idx] / other[degree_idx]);
          },
          flexible_type(0.15),
          edge_direction::IN_EDGE);
      for (size_t i = 0; i < g.get_num_partitions(); ++i) {
        vdata[i] = vdata[i].replace_column(ret[i], "data");
      }
      // g.get_vertices().debug_print();
    }
  }

  void test_pagerank() {
    size_t n_vertex = 10;
    size_t n_partition = 2;
    {
      // for symmetic ring graph, all vertices should have the same pagerank
      sgraph ring_graph  = create_ring_graph(n_vertex, n_partition, false);
      compute_pagerank(ring_graph, 3);
      sframe vdata = ring_graph.get_vertices();
      size_t data_column_index = vdata.column_index("data");
      std::vector<std::vector<flexible_type>> vdata_buffer;
      vdata.get_reader()->read_rows(0, ring_graph.num_vertices(), vdata_buffer);
      for (auto& row : vdata_buffer) {
        TS_ASSERT_EQUALS(row[data_column_index], 1.0);
      }
    }
    {
      // for star graph, the center's pagerank = 0.15 + 0.85 * (n-1)) 
      sgraph star_graph = create_star_graph(n_vertex, n_partition);
      star_graph.get_edges().debug_print();
      compute_pagerank(star_graph, 3);
      sframe vdata = star_graph.get_vertices();
      vdata.debug_print();
      size_t id_column_index = vdata.column_index("__id");
      size_t data_column_index = vdata.column_index("data");
      std::vector<std::vector<flexible_type>> vdata_buffer;
      vdata.get_reader()->read_rows(0, star_graph.num_vertices(), vdata_buffer);
      for (auto& row : vdata_buffer) {
        if (row[id_column_index] == 0) {
          double expected = 0.15 + 0.85 * 0.15 * (n_vertex-1);
          TS_ASSERT_DELTA(row[data_column_index], expected, 0.0001);
        } else {
          TS_ASSERT_DELTA(row[data_column_index], 0.15, 0.0001);
        }
      }
    }
  }


  void check_vertex_apply_result(std::vector<std::shared_ptr<sarray<flexible_type>>>& val) {
    for(auto& iter: val) {
      std::vector<flexible_type> ret;
      turi::copy(*iter, std::inserter(ret, ret.end()));
      for (auto& retval: ret) {
        TS_ASSERT_EQUALS((int)retval.get_type(), (int)flex_type_enum::FLOAT);
        TS_ASSERT_EQUALS(retval.get<double>(), 2.0);
      }
    }
  }

  void test_vertex_apply() {
    // there are 4 overloads
    size_t n_vertex = 10;
    size_t n_partition = 2;
    sgraph ring_graph  = create_ring_graph(n_vertex, n_partition, false);
    size_t data_index = ring_graph.vertex_group(0)[0].column_index("data");
    // map data + 1 = 2.0
    auto ret = sgraph_compute::vertex_apply(ring_graph,
                                            flex_type_enum::FLOAT,
                                            [=](const std::vector<flexible_type>& val){
                                              TS_ASSERT_LESS_THAN(data_index, val.size());
                                              return val[data_index] + 1.0;
                                            });
    check_vertex_apply_result(ret);


    // map data + prevret / 2 = 2.0
    ret = sgraph_compute::vertex_apply(ring_graph,
                                       ret,
                                       flex_type_enum::FLOAT,
                                       [=](const std::vector<flexible_type>& val, flexible_type prev_ret){
                                         TS_ASSERT_LESS_THAN(data_index, val.size());
                                         return val[data_index] + prev_ret / 2; 
                                       });
    check_vertex_apply_result(ret);



    // map data + prevret / 2 = 2.0
    ret = sgraph_compute::vertex_apply(ring_graph,
                                       "data",
                                       ret,
                                       flex_type_enum::FLOAT,
                                       [=](const flexible_type& val, flexible_type prev_ret){
                                         return val + prev_ret / 2; 
                                       });
    check_vertex_apply_result(ret);


    // map data + 1 = 2.0
    ret = sgraph_compute::vertex_apply(ring_graph,
                                       "data",
                                       flex_type_enum::FLOAT,
                                       [=](const flexible_type& val){
                                         return val + 1.0;
                                       });
    check_vertex_apply_result(ret);

    double vsum = sgraph_compute::vertex_reduce<double>(ring_graph,
                                        [=](const std::vector<flexible_type>& val, double& sum) {
                                          TS_ASSERT_LESS_THAN(data_index, val.size());
                                          sum += (double)val[data_index];
                                        }, 
                                        [=](const double& val, double& sum) {
                                          sum += val; 
                                        });
    TS_ASSERT_EQUALS(vsum, n_vertex);


    flexible_type vsum2 = sgraph_compute::vertex_reduce<flexible_type>(ring_graph,
                                                 "data",
                                                 [=](const flexible_type& val, flexible_type& sum) {
                                                   sum += val;
                                                 },
                                                 [=](const flexible_type& val, flexible_type& sum) {
                                                   sum += val;
                                                 });
    TS_ASSERT_EQUALS(vsum2, n_vertex);

  }
};

BOOST_FIXTURE_TEST_SUITE(_sgraph_compute_test, sgraph_compute_test)
BOOST_AUTO_TEST_CASE(test_triple_apply_edge_data_modification) {
  sgraph_compute_test::test_triple_apply_edge_data_modification();
}
BOOST_AUTO_TEST_CASE(test_basic_edge_count) {
  sgraph_compute_test::test_basic_edge_count();
}
BOOST_AUTO_TEST_CASE(test_pagerank) {
  sgraph_compute_test::test_pagerank();
}
BOOST_AUTO_TEST_CASE(test_vertex_apply) {
  sgraph_compute_test::test_vertex_apply();
}
BOOST_AUTO_TEST_SUITE_END()
