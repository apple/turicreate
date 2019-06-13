/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/lazy_eval/lazy_eval_operation_dag.hpp>
#include <core/storage/lazy_eval/lazy_eval_operation.hpp>
#include <core/storage/sframe_interface/unity_sgraph_lazy_ops.hpp>
#include <model_server/lib/unity_global.hpp>
#include <model_server/lib/unity_global_singleton.hpp>
#include <model_server/lib/variant.hpp>
#include <core/storage/sgraph_data/sgraph_io.hpp>
#include <core/storage/sgraph_data/sgraph_triple_apply.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/parallel/mutex.hpp>
#include <boost/range/adaptors.hpp>
#include <functional>



/**************************************************************************/
/*                                                                        */
/*                            Unity Graph Impl                            */
/*                                                                        */
/**************************************************************************/
namespace turi {

const char* unity_sgraph::GRAPH_MAGIC_HEADER = "GLSGRAPH";

unity_graph_dag_type* unity_sgraph::dag_singleton= NULL;

static mutex dag_access_mutex;

unity_graph_dag_type* unity_sgraph::get_dag() {
  if (!dag_singleton) {
    dag_singleton = new unity_graph_dag_type(
        [](){
          return new sgraph(SGRAPH_DEFAULT_NUM_PARTITIONS);
        },
        [](sgraph& dst, sgraph& src) { dst = src; }
    );
  }
  return dag_singleton;
}

unity_sgraph::unity_sgraph(size_t npartitions) {
  std::lock_guard<mutex> lock(dag_access_mutex);
  m_graph.reset(unity_sgraph::get_dag()->add_value(new sgraph(npartitions)));
}

unity_sgraph::unity_sgraph(std::shared_ptr<sgraph> g) {
  std::lock_guard<mutex> lock(dag_access_mutex);
  m_graph.reset(unity_sgraph::get_dag()->add_value(g));
}

unity_sgraph::~unity_sgraph() { }

std::shared_ptr<unity_sgraph_base> unity_sgraph::clone() {
  log_func_entry();
  return std::make_shared<unity_sgraph>(*this);
}

std::shared_ptr<unity_sframe_base> unity_sgraph::get_vertices(
    const std::vector<flexible_type>& vid_vec,
    const options_map_t& field_constraint, size_t group) {
  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  ret->set_sframe(std::make_shared<sframe>(
          (*m_graph)().get_vertices(vid_vec, field_constraint, group)));
  return ret;
}

std::shared_ptr<unity_sframe_base> unity_sgraph::get_edges(
                        const std::vector<flexible_type>& source_vids,
                        const std::vector<flexible_type>& target_vids,
                        const options_map_t& field_constraint,
                        size_t groupa, size_t groupb) {
  // fast track for simple query
  if ((source_vids.size() + target_vids.size() + field_constraint.size()) == 0) {
    return get_edges_lazy(groupa, groupb);
  } else {
    std::shared_ptr<unity_sframe> ret(new unity_sframe());
    ret->set_sframe(std::make_shared<sframe>(
          (*m_graph)().get_edges(source_vids, target_vids, field_constraint, groupa, groupb)));
    return ret;
  }
}

struct lazy_id_translation_functor {
  lazy_id_translation_functor() { }

  lazy_id_translation_functor(std::shared_ptr<const std::vector<flexible_type>> id_vec) {
    m_id_vec = id_vec;
  }

  operator int() const { return 1; }

  flexible_type operator()(const flexible_type& i) const{
    DASSERT_LT(i, m_id_vec->size());
    return m_id_vec->at(i);
  }

  std::shared_ptr<const std::vector<flexible_type>> m_id_vec;
};

/**
 * Helper function: recursively append the a vector of lazy objects. Return the result of
 * append in the range(begin, end)
 */
std::shared_ptr<unity_sframe_base> binary_sframe_append(
  const std::vector<std::shared_ptr<unity_sframe_base>>& lazy_sframes,
  size_t begin_index, size_t end_index) {

  std::shared_ptr<unity_sframe_base> ret;
  if (begin_index + 1 == end_index)
    ret = lazy_sframes[begin_index];
  else if (begin_index < end_index) {
    size_t middle = begin_index + (end_index - begin_index)/ 2;
    auto left = binary_sframe_append(lazy_sframes, begin_index, middle);
    auto right = binary_sframe_append(lazy_sframes, middle, end_index);
    if (left && right) {
      ret = left->append(right);
    } else if (left) {
      ret= left;
    } else {
      ret = right;
    }
  }
  return ret;
}

std::shared_ptr<unity_sframe_base> unity_sgraph::get_edges_lazy(size_t groupa, size_t groupb) {
  std::shared_ptr<unity_sframe> ret(new unity_sframe());
  auto& g = (*m_graph)();

  if (g.num_edges(groupa, groupb) == 0) {
    sframe sf;
    sf.open_for_write(get_edge_fields(), get_edge_field_types());
    sf.close();
    ret->construct_from_sframe(sf);
    return ret;
  }

  size_t num_partitions = g.get_num_partitions();
  std::vector<sframe>& egroup = g.edge_group(groupa, groupb);
  std::vector<sframe>& vdata_groupa = g.vertex_group(groupa);
  std::vector<sframe>& vdata_groupb = g.vertex_group(groupb);

  std::vector<std::string> edge_column_names = g.get_edge_fields();
  std::vector<flex_type_enum> edge_column_types = g.get_edge_field_types();
  flex_type_enum id_type = g.vertex_id_type();

  std::map<std::pair<size_t, size_t>, lazy_id_translation_functor> id_column_translators;

  std::vector<std::shared_ptr<unity_sframe_base>> edge_partition_sframes;

  for (size_t i = 0; i < num_partitions; ++i) {
    for (size_t j = 0; j < num_partitions; ++j) {

      auto& eframe = egroup[i * num_partitions + j];

      // make the id transform functor for source and target id columns
      if (!id_column_translators.count({groupa, i})) {
        std::shared_ptr<sarray<flexible_type>> source_id_column = vdata_groupa[i].select_column(sgraph::VID_COLUMN_NAME);
        std::vector<flexible_type> id_vec;
        source_id_column->get_reader()->read_rows(0, source_id_column->size(), id_vec);
        std::shared_ptr<const std::vector<flexible_type>> id_vec_ptr =
          std::make_shared<const std::vector<flexible_type>>(id_vec);
        id_column_translators[{groupa, i}] = lazy_id_translation_functor(id_vec_ptr);
      }
      if (!id_column_translators.count({groupb, j})) {
        std::shared_ptr<sarray<flexible_type>> target_id_column = vdata_groupb[j].select_column(sgraph::VID_COLUMN_NAME);
        std::vector<flexible_type> id_vec;
        target_id_column->get_reader()->read_rows(0, target_id_column->size(), id_vec);
        std::shared_ptr<const std::vector<flexible_type>> id_vec_ptr =
          std::make_shared<const std::vector<flexible_type>>(id_vec);
        id_column_translators[{groupb, j}] = lazy_id_translation_functor(id_vec_ptr);
      }

      // construct the lazy source and target sarrays.
      std::shared_ptr<unity_sarray> source_array, target_array;
      source_array = std::make_shared<unity_sarray>();
      source_array->construct_from_sarray(eframe.select_column(sgraph::SRC_COLUMN_NAME));
      target_array = std::make_shared<unity_sarray>();
      target_array->construct_from_sarray(eframe.select_column(sgraph::DST_COLUMN_NAME));
      auto source_translator = id_column_translators[{groupa, i}];
      auto target_translator = id_column_translators[{groupb, j}];
      auto lazy_source_array = source_array->transform_lambda(
        [=](const flexible_type& local_vid) { return source_translator(local_vid); },
        id_type, false, 0);
      auto lazy_target_array = target_array->transform_lambda(
        [=](const flexible_type& local_vid) { return target_translator(local_vid); },
        id_type, false, 0);

      // Construct new sframe for the edge segment, replacing the raw ids with new ids.
      auto new_eframe = std::make_shared<unity_sframe>();
      size_t src_column_idx = eframe.column_index(sgraph::SRC_COLUMN_NAME);
      size_t dst_column_idx = eframe.column_index(sgraph::DST_COLUMN_NAME);
      for (size_t i = 0; i < eframe.num_columns(); ++i) {
        if (i == src_column_idx) {
          new_eframe->add_column(lazy_source_array, sgraph::SRC_COLUMN_NAME);
        } else if (i == dst_column_idx) {
          new_eframe->add_column(lazy_target_array, sgraph::DST_COLUMN_NAME);
        } else {
          auto column_data = std::make_shared<unity_sarray>();
          column_data->construct_from_sarray(eframe.select_column(i));
          auto column_name = eframe.column_names()[i];
          new_eframe->add_column(column_data, column_name);
        }
      }
      edge_partition_sframes.push_back(std::static_pointer_cast<unity_sframe_base>(new_eframe));

    }
  }

  // now we want to append N = num_partitions * num_partiions lazy sframes into one final sframe and
  // with append tree depth log(N)
  auto lazy_append_edges = binary_sframe_append(edge_partition_sframes, 0, edge_partition_sframes.size());
  return lazy_append_edges;
}

options_map_t unity_sgraph::summary() {
  log_func_entry();
  options_map_t ret;
  auto& g = (*m_graph)();
  ret["num_vertices"] = g.num_vertices();
  ret["num_edges"] = g.num_edges();
  return ret;
}

std::vector<std::string> unity_sgraph::get_vertex_fields(size_t group) {
  return (*m_graph)().get_vertex_fields(group);
}

std::vector<std::string> unity_sgraph::get_edge_fields(size_t groupa, size_t groupb) {
  return (*m_graph)().get_edge_fields(groupa, groupb);
}

std::vector<flex_type_enum> unity_sgraph::get_vertex_field_types(size_t group) {
  return (*m_graph)().get_vertex_field_types(group);
}

std::vector<flex_type_enum> unity_sgraph::get_edge_field_types(size_t groupa, size_t groupb) {
  return (*m_graph)().get_edge_field_types(groupa, groupb);
}

std::shared_ptr<unity_sgraph_base> unity_sgraph::add_vertices(
    std::shared_ptr<unity_sframe_base> vertices,
    const std::string& id_field_name,
    size_t group) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  std::shared_ptr<unity_sframe> unity_sf = std::static_pointer_cast<unity_sframe>(vertices);
  ASSERT_TRUE(unity_sf != NULL);
  std::shared_ptr<sframe> sf = unity_sf->get_underlying_sframe();

  fast_validate_add_vertices(*sf, id_field_name, group);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->
      add_operation(new add_vertices_op<sframe>(sf, id_field_name, group), {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base> unity_sgraph::add_edges(
    std::shared_ptr<unity_sframe_base> edges,
    const std::string& source_field_name,
    const std::string& target_field_name,
    size_t groupa, size_t groupb) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  std::shared_ptr<unity_sframe> unity_sf = std::static_pointer_cast<unity_sframe>(edges);
  ASSERT_TRUE(unity_sf != NULL);
  std::shared_ptr<sframe> sf = unity_sf->get_underlying_sframe();

  fast_validate_add_edges(*sf, source_field_name, target_field_name, groupa, groupb);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
          new add_edges_op<sframe>(sf, source_field_name, target_field_name, groupa, groupb),
                                   {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base> unity_sgraph::copy_vertex_field(
    const std::string field,
    const std::string newfield,
    size_t group) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == newfield) {
    log_and_throw("Cannot copy to the same field.");
  }
  if (newfield == sgraph::VID_COLUMN_NAME) {
    log_and_throw("Cannot copy to required field " + newfield);
  }

  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new copy_vertex_field_op(field, newfield, group),
        {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::delete_vertex_field(const std::string field, size_t group) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == sgraph::VID_COLUMN_NAME) {
    log_and_throw("Cannot delete required field " + field);
  }

  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new delete_vertex_field_op(field, group),
        {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base> unity_sgraph::add_vertex_field(
    std::shared_ptr<unity_sarray_base> in_column_data, std::string field) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == sgraph::VID_COLUMN_NAME) {
    log_and_throw("Cannot add id field " + field);
  }
  sgraph* new_graph = new sgraph((*m_graph)());
  std::shared_ptr<unity_sarray> column_data =
      std::static_pointer_cast<unity_sarray>(in_column_data);
  new_graph->add_vertex_field(column_data->get_underlying_sarray(), field);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::swap_vertex_fields(const std::string& field1, const std::string& field2) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field1 == sgraph::VID_COLUMN_NAME || field2 == sgraph::VID_COLUMN_NAME) {
    log_and_throw("Cannot swap id fields " + field1 + " , " + field2);
  }
  sgraph* new_graph = new sgraph((*m_graph)());
  new_graph->swap_vertex_fields(field1, field2);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::rename_vertex_fields(const std::vector<std::string>& oldnames,
                                   const std::vector<std::string>& newnames) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  sgraph* new_graph = new sgraph((*m_graph)());
  new_graph->rename_vertex_fields(oldnames, newnames);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base> unity_sgraph::select_vertex_fields(
    const std::vector<std::string>& fields, size_t group) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  std::vector<std::string> fields_with_id({sgraph::VID_COLUMN_NAME});
  fields_with_id.insert(fields_with_id.end(), fields.begin(), fields.end());
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new select_vertex_fields_op(fields_with_id, group), {m_graph.get()}));

  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::copy_edge_field(const std::string field,
                              const std::string newfield,
                              size_t groupa, size_t groupb) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == newfield) {
    log_and_throw("Cannot copy to the same field");
  }
  if (newfield == sgraph::SRC_COLUMN_NAME ||
      newfield == sgraph::DST_COLUMN_NAME) {
    log_and_throw("Cannot copy to required field " + newfield);
  }
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new copy_edge_field_op(field, newfield, groupa, groupb),
        {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::delete_edge_field(const std::string field,
                                size_t groupa, size_t groupb) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == sgraph::SRC_COLUMN_NAME ||
      field == sgraph::DST_COLUMN_NAME) {
    log_and_throw("Cannot remove required field " + field);
  }
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new delete_edge_field_op(field, groupa, groupb),
        {m_graph.get()}));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::add_edge_field(std::shared_ptr<unity_sarray_base> in_column_data,
                             std::string field) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field == sgraph::SRC_COLUMN_NAME ||
      field == sgraph::DST_COLUMN_NAME) {
    log_and_throw("Cannot add id field " + field);
  }
  sgraph* new_graph = new sgraph((*m_graph)());
  std::shared_ptr<unity_sarray> column_data =
      std::static_pointer_cast<unity_sarray>(in_column_data);
  new_graph->add_edge_field(column_data->get_underlying_sarray(), field);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::swap_edge_fields(const std::string& field1,
                               const std::string& field2) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  if (field1 == sgraph::SRC_COLUMN_NAME || field2 == sgraph::SRC_COLUMN_NAME ||
      field1 == sgraph::DST_COLUMN_NAME || field2 == sgraph::DST_COLUMN_NAME) {
    log_and_throw("Cannot swap id fields " + field1 + " , " + field2);
  }
  sgraph* new_graph = new sgraph((*m_graph)());
  new_graph->swap_edge_fields(field1, field2);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::rename_edge_fields(const std::vector<std::string>& oldnames,
                                 const std::vector<std::string>& newnames) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  sgraph* new_graph = new sgraph((*m_graph)());
  new_graph->rename_edge_fields(oldnames, newnames);
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_value(new_graph));
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::select_edge_fields(const std::vector<std::string>& fields,
                                 size_t groupa, size_t groupb) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  std::vector<std::string> fields_with_id({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
  fields_with_id.insert(fields_with_id.end(), fields.begin(), fields.end());
  std::shared_ptr<unity_sgraph> g(new unity_sgraph(*this));
  g->m_graph.reset(unity_sgraph::get_dag()->add_operation(
        new select_edge_fields_op(fields_with_id, groupa, groupb),
        {m_graph.get()}));
  std::map<std::string, flex_type_enum> new_field_type_map;
  return g;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::lambda_triple_apply(const std::string& lambda_str,
                                  const std::vector<std::string>& mutated_fields) {
#ifdef TC_HAS_PYTHON
  log_func_entry();
  if (mutated_fields.empty()) {
    log_and_throw("mutated_fields cannot be empty");
  }
  std::shared_ptr<sgraph> g = std::make_shared<sgraph>((*m_graph)());
  std::vector<std::string> mutated_vertex_fields, mutated_edge_fields;
  const auto& all_vertex_fields = g->get_vertex_fields();
  const auto& all_edge_fields = g->get_edge_fields();
  std::set<std::string> all_vertex_field_set(all_vertex_fields.begin(), all_vertex_fields.end());
  std::set<std::string> all_edge_field_set(all_edge_fields.begin(), all_edge_fields.end());
  for (auto& f : mutated_fields) {
    if (f == sgraph::VID_COLUMN_NAME || f == sgraph::SRC_COLUMN_NAME || f == sgraph::DST_COLUMN_NAME) {
      log_and_throw("mutated fields cannot contain id field: " + f);
    }
    if (!all_vertex_field_set.count(f) && !all_edge_field_set.count(f)) {
      log_and_throw("mutated field \"" + f + "\" cannot be found in graph");
    }
    if (all_vertex_field_set.count(f))
      mutated_vertex_fields.push_back(f);
    if (all_edge_field_set.count(f))
      mutated_edge_fields.push_back(f);
  }
  DASSERT_FALSE(mutated_fields.empty());
  sgraph_compute::triple_apply(*g, lambda_str, mutated_vertex_fields, mutated_edge_fields);
  std::shared_ptr<unity_sgraph> ret(new unity_sgraph(g));
  return ret;
#else
  log_and_throw("Python functions not supported");
#endif
}


std::shared_ptr<unity_sgraph_base>
unity_sgraph::lambda_triple_apply_native(const lambda_triple_apply_fn& lambda,
                                         const std::vector<std::string>& mutated_fields) {
  log_func_entry();
  if (mutated_fields.empty()) {
    log_and_throw("mutated_fields cannot be empty");
  }
  std::shared_ptr<sgraph> g = std::make_shared<sgraph>((*m_graph)());
  std::vector<std::string> mutated_vertex_fields, mutated_edge_fields;
  const auto& all_vertex_fields = g->get_vertex_fields();
  const auto& all_edge_fields = g->get_edge_fields();
  std::vector<size_t> mutated_vertex_field_ids;
  std::vector<size_t> mutated_edge_field_ids;
  std::set<std::string> all_vertex_field_set(all_vertex_fields.begin(), all_vertex_fields.end());
  std::set<std::string> all_edge_field_set(all_edge_fields.begin(), all_edge_fields.end());
  for (auto& f : mutated_fields) {
    if (f == sgraph::VID_COLUMN_NAME || f == sgraph::SRC_COLUMN_NAME || f == sgraph::DST_COLUMN_NAME) {
      log_and_throw("mutated fields cannot contain id field: " + f);
    }
    if (!all_vertex_field_set.count(f) && !all_edge_field_set.count(f)) {
      log_and_throw("mutated field \"" + f + "\" cannot be found in graph");
    }
    if (all_vertex_field_set.count(f)) {
      mutated_vertex_fields.push_back(f);
      mutated_vertex_field_ids.push_back(std::find(all_vertex_fields.begin(),
                                                   all_vertex_fields.end(), f) -
                                          all_vertex_fields.begin());
    }
    if (all_edge_field_set.count(f)) {
      mutated_edge_fields.push_back(f);
      mutated_edge_field_ids.push_back(std::find(all_edge_fields.begin(),
                                                 all_edge_fields.end(), f) -
                                       all_edge_fields.begin());
    }
  }
  DASSERT_FALSE(mutated_fields.empty());
  // get all the field names in flexible_type form since we can do CoW on it
  std::vector<flexible_type> flex_vertex_fields(all_vertex_fields.begin(),
                                                all_vertex_fields.end());
  std::vector<flexible_type> flex_edge_fields(all_edge_fields.begin(),
                                              all_edge_fields.end());
  auto new_lambda =
      [=](sgraph_compute::edge_scope& e)->void {
        e.lock_vertices();
        edge_triple triple;
        for (size_t i = 0;i < e.source().size(); ++i) {
          triple.source[flex_vertex_fields[i]] = e.source()[i];
          triple.target[flex_vertex_fields[i]] = e.target()[i];
        }
        for (size_t i = 0;i < e.edge().size(); ++i) {
          triple.edge[flex_edge_fields[i]] = e.edge()[i];
        }

        lambda(triple);

        // update just the potentially changed fields
        for (size_t vtxfield : mutated_vertex_field_ids) {
          e.source()[vtxfield] = std::move(triple.source[flex_vertex_fields[vtxfield]]);
          e.target()[vtxfield] = std::move(triple.target[flex_vertex_fields[vtxfield]]);
        }
        for (size_t edgefield : mutated_edge_field_ids) {
          e.edge()[edgefield] = std::move(triple.edge[flex_edge_fields[edgefield]]);
        }
        e.unlock_vertices();
      };
  sgraph_compute::triple_apply(*g, new_lambda, mutated_vertex_fields, mutated_edge_fields);
  std::shared_ptr<unity_sgraph> ret(new unity_sgraph(g));
  return ret;
}


flexible_type _map_to_flex_dict(std::map<std::string, flexible_type>&& map) {
  flex_dict ret;
  ret.reserve(map.size());
  for (auto&& pair: map)
    ret.push_back(std::make_pair(flexible_type(pair.first), pair.second));
  return ret;
}

std::map<std::string, flexible_type> _map_from_flex_dict(flex_dict&& dict) {
  std::map<std::string, flexible_type> ret;
  for (auto&& pair: dict)
    ret[pair.first] = pair.second;
  return ret;
}

std::shared_ptr<unity_sgraph_base>
unity_sgraph::lambda_triple_apply_native(const function_closure_info& toolkit_fn_name,
                                  const std::vector<std::string>& mutated_fields) {
  auto native_execute_function =
                  get_unity_global_singleton()
                  ->get_toolkit_function_registry()
                  ->get_native_function(toolkit_fn_name);

  log_func_entry();

  auto lambda = [=](edge_triple& args)->void {
    std::vector<variant_type> var(3);
    var[0] = to_variant(_map_to_flex_dict(std::move(args.source)));
    var[1] = to_variant(_map_to_flex_dict(std::move(args.edge)));
    var[2] = to_variant(_map_to_flex_dict(std::move(args.target)));

    variant_type ret = native_execute_function(var);
    var = variant_get_value<std::vector<variant_type>>(ret);

    args.source = _map_from_flex_dict(variant_get_value<flexible_type>(var[0]));
    args.edge = _map_from_flex_dict(variant_get_value<flexible_type>(var[1]));
    args.target = _map_from_flex_dict(variant_get_value<flexible_type>(var[2]));
  };

  return lambda_triple_apply_native(lambda, mutated_fields);
}


sgraph& unity_sgraph::get_graph() const {
  ASSERT_TRUE(m_graph);
  return (*m_graph)();
};

void unity_sgraph::save(oarchive& oarc) const {
  log_func_entry();
  oarc.write(GRAPH_MAGIC_HEADER, strlen(GRAPH_MAGIC_HEADER));
  oarc << get_graph().get_num_partitions() << get_graph();
}

void unity_sgraph::save_reference(oarchive& oarc) const {
  log_func_entry();
  oarc.write(GRAPH_MAGIC_HEADER, strlen(GRAPH_MAGIC_HEADER));
  oarc << get_graph().get_num_partitions();
  get_graph().save_reference(oarc);
}

void unity_sgraph::load(iarchive& iarc) {
  log_func_entry();
  std::lock_guard<mutex> lock(dag_access_mutex);
  char buf[256] = "";
  size_t magic_header_size = strlen(GRAPH_MAGIC_HEADER);
  iarc.read(buf, magic_header_size);
  if (strcmp(buf, GRAPH_MAGIC_HEADER)) {
    log_and_throw(std::string("Invalid graph file."));
  }
  size_t num_partitions = 0;
  iarc >> num_partitions;
  sgraph* g = new sgraph(num_partitions);
  iarc >> *g;
  m_graph.reset(unity_sgraph::get_dag()->add_value(g));
}

void unity_sgraph::save_reference(std::string target_dir) const {
  dir_archive dir;
  dir.open_directory_for_write(target_dir);
  dir.set_metadata("contents", "graph");
  oarchive oarc(dir);
  if (dir.get_output_stream()->fail()) {
    log_and_throw_io_failure("Fail to write");
  }
  save_reference(oarc);
  dir.close();
}

bool unity_sgraph::save_graph(std::string target, std::string format) {
  log_func_entry();
  try {
    if (format == "binary") {
      dir_archive dir;
      dir.open_directory_for_write(target);
      dir.set_metadata("contents", "graph");
      oarchive oarc(dir);
      if (dir.get_output_stream()->fail()) {
        log_and_throw_io_failure("Fail to write");
      }
      save(oarc);
      dir.close();
    } else if (format == "json") {
      save_sgraph_to_json(get_graph(), target);
    } else if (format == "csv") {
      save_sgraph_to_csv(get_graph(), target);
    } else {
      log_and_throw("Unable to save to format : " + format);
    }
  } catch (std::ios_base::failure& e) {
    std::string message =
        "Unable to save graph to " + sanitize_url(target) + ": " + e.what();
    log_and_throw_io_failure(message);
  } catch (std::string& e) {
    std::string message =
        "Unable to save graph to " + sanitize_url(target) + ": " + e;
    log_and_throw(message);
  } catch (...) {
    std::string message =
        "Unable to save graph to " + sanitize_url(target) + ": Unknown Error.";
    log_and_throw(message);
  }
  return true;
}

bool unity_sgraph::load_graph(std::string target_dir) {
  log_func_entry();
  try {
    dir_archive dir;
    dir.open_directory_for_read(target_dir);
    std::string contents;
    if (dir.get_metadata("contents", contents) == false ||
        contents != "graph") {
      log_and_throw(std::string("Archive does not contain a graph."));
    }
    iarchive iarc(dir);
    load(iarc);
    dir.close();
  } catch (std::ios_base::failure& e) {
    std::string message = "Unable to load graph from " + sanitize_url(target_dir)
      + ": " + e.what();
    log_and_throw_io_failure(message);
  } catch (std::string& e) {
    std::string message = "Unable to load graph from " + sanitize_url(target_dir)
      + ": " + e;
    log_and_throw(message);
  } catch (...) {
    std::string message = "Unable to load graph from " + sanitize_url(target_dir)
      + ": Unknown Error.";
    log_and_throw(message);
  }
  return true;
}

void unity_sgraph::fast_validate_add_vertices(const sframe& vertices,
    std::string id_field, size_t group) const {
  if (!vertices.contains_column(id_field)) {
    log_and_throw("Input sframe does not contain id column: " + id_field);
  }
  flex_type_enum id_type = vertices.column_type(vertices.column_index(id_field));

  if (id_type != flex_type_enum::INTEGER && id_type != flex_type_enum::STRING) {
    log_and_throw(
        std::string("Invalid id column type : ")
        + flex_type_enum_to_name(id_type)
        + ". Supported types are: integer and string."
    );
  }
}

void unity_sgraph::fast_validate_add_edges(const sframe& edges,
    std::string src_field,
    std::string dst_field,
    size_t groupa, size_t groupb) const {

  if (!edges.contains_column(src_field)) {
    log_and_throw("Input sframe does not contain source id column: " + src_field);
  }
  if (!edges.contains_column(dst_field)) {
    log_and_throw("Input sframe does not contain target id column: " + dst_field);
  }

  flex_type_enum src_id_type = edges.column_type(edges.column_index(src_field));
  flex_type_enum dst_id_type = edges.column_type(edges.column_index(dst_field));

  if (src_id_type != dst_id_type) {
    std::string msg = "Source and target ids have different types: ";
    msg += std::string(flex_type_enum_to_name(src_id_type)) + " != " + flex_type_enum_to_name(dst_id_type);
    log_and_throw(msg);
  }

  if (src_id_type != flex_type_enum::INTEGER && src_id_type != flex_type_enum::STRING) {
    log_and_throw(
        std::string("Invalid id column type : ")
        + flex_type_enum_to_name(src_id_type)
        + ". Supported types are: integer and string."
    );
  }
}

} // namespace turi
