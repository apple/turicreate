/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// rpc and distributed
#include <rpc/dc.hpp>
#include <rpc/dc_dist_object.hpp>
#include <rpc/dc_global.hpp>
#include <distributed/distributed_context.hpp>
#include <unity/dml/dml_function_invocation.hpp>
#include <unity/dml/dml_function_wrapper.hpp>

// SGraph
#include <sframe/sframe.hpp>
#include <sgraph/sgraph.hpp>
#include <sgraph/sgraph_compute.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>
#include <sgraph/hilbert_curve.hpp>
#include <table_printer/table_printer.hpp>
// Distributed Graph
#include <unity/dml/distributed_graph.hpp>
#include <unity/dml/distributed_graph_compute.hpp>

// Unity data structures
#include <unity/lib/gl_sgraph.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/simple_model.hpp>

// Parallel for
#include <parallel/pthread_tools.hpp>

// EIGTODO
#include <numerics/armadillo.hpp>

namespace turi {

const int MAX_CLASSES = 1000;

struct graph_label_info {

  size_t num_unlabeled_vertices = 0;
  size_t num_labeled_vertices = 0;
  size_t min_class = std::numeric_limits<size_t>::max();
  size_t max_class = std::numeric_limits<size_t>::min();
  size_t num_classes = 0;

  graph_label_info& operator+=(const graph_label_info& other) {
    num_unlabeled_vertices += other.num_unlabeled_vertices;
    num_labeled_vertices += other.num_labeled_vertices;
    min_class = std::min(min_class, other.min_class);
    max_class = std::max(max_class, other.max_class);
    return *this;
  }

  void save(oarchive& oarc) const {
    oarc << num_unlabeled_vertices << num_labeled_vertices
         << min_class << max_class << num_classes;
  }
  void load(iarchive& iarc) {
    iarc >> num_unlabeled_vertices >> num_labeled_vertices
         >> min_class >> max_class >> num_classes;
  }

};

/**
 * Graph related constant data
 */
graph_label_info get_label_info(distributed_sgraph_compute::distributed_graph& g,
                                std::string label_field,
                                distributed_control* dc) {

  size_t num_partitions = g.num_partitions();

  std::vector<graph_label_info> label_info_by_partition(num_partitions);

  /// scan the label column, and update aggregated label info.
  auto scan_label = [&] (const std::vector<flexible_type>& labels,
                         size_t partition_id) {

    graph_label_info& info = label_info_by_partition[partition_id];
    for (const auto& label: labels) {
      if (label.is_na()) {
        ++info.num_unlabeled_vertices;
      } else {
        ++info.num_labeled_vertices;
        info.min_class = std::min(info.min_class, (size_t)label);
        info.max_class = std::max(info.max_class, (size_t)label);
      }
    }
  };

  for (auto partition_id: g.my_master_vertex_partitions()) {
    std::vector<flexible_type> vdata;
    auto vdata_sa = g.local_graph().vertex_partition(partition_id).select_column(label_field);
    vdata_sa->get_reader()->read_rows(0, vdata_sa->size(), vdata);
    scan_label(vdata, partition_id);
  }

  // Local partial aggregate
  graph_label_info aggregated_info = std::accumulate(
    label_info_by_partition.begin(),
    label_info_by_partition.end(),
    graph_label_info(),
    [](graph_label_info a, graph_label_info b) {
      a += b;
      return a;
    });

  // Distributed aggregate
  dc->all_reduce(aggregated_info);
  aggregated_info.num_classes = aggregated_info.max_class + 1;

  // Sanity check
  if (aggregated_info.min_class != 0) {
    log_and_throw("Class labels must be [0, num_classes)");
  }
  DASSERT_EQ(aggregated_info.num_labeled_vertices + aggregated_info.num_unlabeled_vertices,
             g.num_vertices());

  logprogress_stream << "Num classes: " << aggregated_info.num_classes << std::endl;
  logprogress_stream << "#labeled_vertices: " << aggregated_info.num_labeled_vertices
                     << "\t#unlabeled_vertices: " << aggregated_info.num_unlabeled_vertices
                     << std::endl;

  if (aggregated_info.num_unlabeled_vertices == 0)
    logprogress_stream << "Warning: all vertices are already labeled" << std::endl;
  if (aggregated_info.num_classes == 1)
    logprogress_stream << "Warning: there are only one classes" << std::endl;

  return aggregated_info;
}

/**************************************************************************/
/*                                                                        */
/*                         Worker Implementation                          */
/*                                                                        */
/**************************************************************************/
std::map<std::string, flexible_type> distributed_labelprop_worker_impl(variant_map_type args)  {

  /// User Input
  std::string graph_path = variant_get_value<flexible_type>(args["__path_of_graph"]);
  double threshold = variant_get_value<double>(args.at("threshold"));
  double self_weight = variant_get_value<double>(args.at("self_weight"));
  bool undirected = variant_get_value<int>(args.at("undirected"));
  flexible_type flex_max_iterations = variant_get_value<flexible_type>(args.at("max_iterations"));
  std::string output_path = variant_get_value<std::string>(args.at("output_path"));

  size_t max_iterations = -1;
  if (flex_max_iterations.get_type() != flex_type_enum::UNDEFINED) {
    max_iterations = flex_max_iterations;
  }
  std::string weight_field, label_field;
  {
    flexible_type tmp = variant_get_value<flexible_type>(args.at("weight_field"));
    if (tmp.get_type() != flex_type_enum::UNDEFINED) {
      weight_field = (std::string)(tmp);
    }
  }
  {
    flexible_type tmp = variant_get_value<flexible_type>(args.at("label_field"));
    if (tmp.get_type() != flex_type_enum::UNDEFINED) {
      label_field = (std::string)(tmp);
    }
  }

  /// Setup In Memory Data Structures ///

  // global_logger().set_log_level(LOG_DEBUG);
  auto dc = distributed_control::get_instance();
  std::vector<std::string> vdata_fields = { label_field };
  std::vector<std::string> edata_fields;
  if (!weight_field.empty()) {
    edata_fields.push_back(weight_field);
  }
  distributed_sgraph_compute::distributed_graph graph(graph_path, dc, vdata_fields, edata_fields);

  // Graph info
  size_t num_partitions = graph.num_partitions();

  // Sanity checking and getting info about the labels.
  auto labels = distributed_sgraph_compute::get_vertex_data_of_master_partitions(graph, label_field);
  auto info = get_label_info(graph, label_field, dc);
  size_t num_classes = info.num_classes;
  if (num_classes > MAX_CLASSES)  {
    log_and_throw("Too many classes provided. Label propagation only works with maximal 1000 classes.");
  }

  // Typedef of EIGTODO Matrix
  typedef EIGTODO::Matrix<double, EIGTODO::Dynamic, EIGTODO::Dynamic, EIGTODO::RowMajor> matrix_type;
  // Initialize vectors of probabilities.
  std::vector<matrix_type> current_label_pb, prev_label_pb;
  std::vector<std::vector<mutex>> vertex_locks;

  current_label_pb = distributed_sgraph_compute::create_partition_aligned_vertex_data<matrix_type>(
      graph,
      [=](size_t num_vertices) { return matrix_type::Zero(num_vertices, num_classes); }
  );
  prev_label_pb = distributed_sgraph_compute::create_partition_aligned_vertex_data<matrix_type>(
      graph,
      [=](size_t num_vertices) { return matrix_type::Zero(num_vertices, num_classes); }
  );
  vertex_locks = distributed_sgraph_compute::create_partition_aligned_vertex_data<std::vector<mutex>>(
      graph,
      [](size_t num_vertices) { return std::vector<mutex>(num_vertices); }
  );

  // Initialize probabilities to 1.0 for vertices with labels, 1/K otherwise.
  static double BASELINE_PROB = 1.0 / num_classes;
  auto vertex_initialization_fun = [&](matrix_type& mat, size_t partition_id) {
      const auto& labels_of_current_partition = labels[partition_id];
      parallel_for(0, mat.n_rows, [&](size_t rowid) {
        flexible_type label = labels_of_current_partition[rowid];
        if (!label.is_na()) {
          if (label >= num_classes) { log_and_throw("Class label must be in [0, numclasses)"); }
          mat.row(rowid).zeros();
          mat(rowid, (size_t)label) = 1.0;
        } else {
          mat.row(rowid).fill(BASELINE_PROB);
        }
      });
  };
  distributed_sgraph_compute::vertex_apply(graph,
                                           prev_label_pb,
                                           vertex_initialization_fun);
  logprogress_stream << "Done initializing label probabilities." << std::endl;

  bool use_edge_weight = !weight_field.empty();
  /**
   * Define one iteration of label propagation in triple apply:
   *
   * \code
   * foreach (src, edge, dst):
   *   current_label_pb[:][dst.addr] += prev_label_pb[:][src.addr] * edge.weight
   * \endcode
   */
  sgraph_compute::fast_triple_apply_fn_type apply_fn =
       [&](sgraph_compute::fast_edge_scope& scope) {
         const auto source_addr = scope.source_vertex_address();
         const auto target_addr = scope.target_vertex_address();
         double weight = use_edge_weight ? (flex_float)scope.edge()[2] : 1.0;

         {
           // Propagate from src to target
           auto delta = prev_label_pb[source_addr.partition_id].row(source_addr.local_id) * weight;
           auto& mtx = vertex_locks[target_addr.partition_id][target_addr.local_id];
           std::lock_guard<mutex> lock(mtx);
           current_label_pb[target_addr.partition_id].row(target_addr.local_id) += delta;
         }

         if (undirected) {
           // Propagate from target to source
           auto delta = prev_label_pb[target_addr.partition_id].row(target_addr.local_id) * weight;
           auto& mtx = vertex_locks[source_addr.partition_id][source_addr.local_id];
           std::lock_guard<mutex> lock(mtx);
           current_label_pb[source_addr.partition_id].row(source_addr.local_id) += delta;
         }
       };

  // Create a reusable combiner for label probabibilities.
  auto labelprop_combiner_fn =
      [](matrix_type& a, const matrix_type& b)->void { a += b; };

  distributed_sgraph_compute::combiner<matrix_type, decltype(labelprop_combiner_fn)>
      labelprop_combiner(*dc, labelprop_combiner_fn);

  /**
   * Iterate until num_iter or convergence:
   * - scale current probabilities by self_weight (1.0 by default)
   * - use triple apply to propagate label probabilities between neighbors
   * - aggregate unnormalized probabilites using a combiner
   * - normalize the label probabilities for each vertex (on local_graph)
   * - compute delta in probabilities to monitor convergence (using all_reduce)
   */
  timer total_ti;
  table_printer table({{"Iteration", 0},
                       {"Average l2 change in class probability", 0},
                       {"Time elapsed", 0.0}});
  table.print_header();


  size_t iter = 0;
  double average_l2_delta = 0.0;
  while (iter < max_iterations) {
    ++iter;
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }

    // Initialize with Zero
    parallel_for(0, num_partitions, [&](size_t i) {
      current_label_pb[i].zeros();
    });

    // Label Propagation
    if (use_edge_weight) {
      distributed_sgraph_compute::fast_triple_apply(graph, apply_fn, {weight_field});
    } else {
      distributed_sgraph_compute::fast_triple_apply(graph, apply_fn);
    }

    // Synchronize vertex label probabilities.
    // If it is a directed propagation, we only need to combine the target
    // vertex partitions.
    logstream(LOG_INFO) << "Perform combine" << std::endl;
    if (undirected) {
      labelprop_combiner.perform_combine(graph, current_label_pb,
                                         distributed_sgraph_compute::combiner_filter::ALL);
    } else {
      labelprop_combiner.perform_combine(graph, current_label_pb,
                                         distributed_sgraph_compute::combiner_filter::DST);
    }


    // Post processing:
    // Add self weight
    // 1. Normalize to probability
    // 2. Clamp labeled data
    // Stores the total l2 diff and label probabilities.
    turi::atomic<float> total_l2_diff = 0.0;

    distributed_sgraph_compute::vertex_apply(graph, current_label_pb,

      [&](matrix_type& current_label_prob_of_partition, size_t partition_id) {

        auto& labels_of_partition = labels[partition_id];
        const matrix_type& prev_label_prob_of_partition = prev_label_pb[partition_id];
        size_t num_vertices_in_partition = labels_of_partition.size();

        // Add self weight the self weight of the previous label value.
        current_label_prob_of_partition += prev_label_prob_of_partition * self_weight;

        parallel_for(0, num_vertices_in_partition, [&](size_t rowid) {
          flexible_type label = labels_of_partition[rowid];
          if (!label.is_na()) {
            // clamp
            current_label_prob_of_partition.row(rowid).zeros();
            current_label_prob_of_partition(rowid, (size_t)label) = 1.0;
          } else {
            // normalize
            current_label_prob_of_partition.row(rowid) /=
                current_label_prob_of_partition.row(rowid).sum();
          }
        }); // end of parallel for
        float diff = (current_label_prob_of_partition - prev_label_prob_of_partition).rowwise().norm().sum();
        total_l2_diff += diff;
      }
    ); // end of vertex apply
    dc->all_reduce(total_l2_diff);

    // Swap the current label and the prev label.
    std::swap(current_label_pb, prev_label_pb);

    // Store iteration and delta. Print progress.
    average_l2_delta = total_l2_diff / info.num_unlabeled_vertices;
    table.print_row(iter, average_l2_delta, total_ti.current_time());

    if (average_l2_delta < threshold) {
      logstream(LOG_INFO) << "Reach convergence" << std::endl;
      break;
    }
  } // end of label_propagation iterations
  table.print_footer();


  // Free some memory
  current_label_pb.clear();
  labels.clear();
  vertex_locks.clear();


  /**
   * Compute predictions
   */
  std::vector<std::vector<flexible_type>> predicted_labels(num_partitions);
  static const double EPSILON = 1e-10;
  // Compute the predicted label by taking the argmax of each probabilty vector
  distributed_sgraph_compute::vertex_apply(graph, predicted_labels,
   [&](std::vector<flexible_type>& prediction, size_t partition_id) {
      size_t num_vertices_in_partition = graph.num_vertices(partition_id);
      predicted_labels[partition_id].resize(num_vertices_in_partition);

      matrix_type& mat = prev_label_pb[partition_id];
      auto& preds = predicted_labels[partition_id];

      parallel_for(0, num_vertices_in_partition, [&](size_t rowid) {
        // Get the index of the largest value in the row, and store it in preds[rowid]
        int best_class_id = -1;
        mat.row(rowid).max(&best_class_id);
        // If we still get uniform distribution, output NONE
        if (fabs(mat(rowid, best_class_id) - BASELINE_PROB) < EPSILON) {
          preds[rowid] = FLEX_UNDEFINED;
        } else {
          preds[rowid] = best_class_id;
        }
      });
   });

  // Write the probability vector back to graph vertex data
  typedef EIGTODO::Map<armadillo>
      vector_buffer_type;

  // output_columns[class_index][partition_index]
  std::vector<std::vector<std::shared_ptr<sarray<flexible_type>>>> output_columns(num_classes);
  for (auto& c : output_columns) c.resize(num_partitions);

  parallel_for(0, num_partitions, [&](size_t i) {
    size_t num_rows = (prev_label_pb)[i].n_rows;

    std::vector<sarray<flexible_type>::iterator>
      out_iterators(num_classes);

    // prepare sarray and writer
    for (size_t k = 0; k < num_classes; ++k) {
      auto sa = std::make_shared<sarray<flexible_type>>();
      sa->open_for_write(1);
      sa->set_type(flex_type_enum::FLOAT);
      out_iterators[k] = sa->get_output_iterator(0);
      output_columns[k][i] = sa;
    }

    // write to sarray
    flex_vec raw_buffer(num_classes);
    vector_buffer_type mapped_buffer(&(raw_buffer[0]),
                                       num_classes);
    for (size_t j = 0; j < num_rows; ++j) {
      mapped_buffer = ((prev_label_pb)[i].row(j)).template cast<double>();
      for (size_t k = 0; k < num_classes; ++k) {
        *(out_iterators[k])++ = raw_buffer[k];
      }
    }

    // close sarray
    for (size_t k = 0; k < num_classes; ++k)
      output_columns[k][i]->close();
  });

  const std::string LABEL_COLUMN_PREFIX = "P";
  const std::string PREDICTED_LABEL_COLUMN_NAME = "predicted_label";

  // Add the predictions and probabilities back to graph vertex data.
  auto predicted_label_sa = std::vector<std::shared_ptr<sarray<flexible_type>>>(num_partitions);
  for (size_t i = 0; i < num_partitions; ++i) {
    auto& sa = predicted_label_sa[i];
    sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write(1);
    sa->set_type(flex_type_enum::INTEGER);
    turi::copy(predicted_labels[i].begin(), predicted_labels[i].end(), *sa);
    sa->close();
  }
  graph.add_vertex_field(predicted_label_sa, PREDICTED_LABEL_COLUMN_NAME, flex_type_enum::INTEGER);

  for (size_t k = 0; k < num_classes; ++k) {
    std::string column_name = LABEL_COLUMN_PREFIX + std::to_string(k);
    graph.add_vertex_field(output_columns[k], column_name, flex_type_enum::FLOAT);
  }

  logprogress_stream << "Saving graph..." << std::endl;
  // Save the graph distributedly
  graph.save_as_sgraph(output_path);
  logprogress_stream << "Done" << std::endl;

  std::map<std::string, flexible_type> return_stats;
  return_stats["average_l2_delta"] = average_l2_delta;
  return_stats["num_iterations"] = iter;
  return return_stats;
}


/**************************************************************************/
/*                                                                        */
/*                        Commander Implementation                        */
/*                                                                        */
/**************************************************************************/
variant_type distributed_labelprop_impl(variant_map_type args) {
  logprogress_stream << "Running distributed label propagation" << std::endl;

  timer mytimer;

  ASSERT_TRUE(args.count("__path_of_graph"));
  std::string path = variant_get_value<flexible_type>(args["__path_of_graph"]);
  // sgraph cannot be passed from commander to worker
  if (args.count("graph")) { args.erase(args.find("graph")); }

  unity_sgraph ug;
  ug.load_graph(path);
  sgraph sg = ug.get_graph();

  std::string output_path = "result_graph";
  if (args.count("__base_path__")) {
    output_path = variant_get_value<std::string>(args["__base_path__"]) + "/" + output_path;
  }
  args["output_path"] = output_path;

  auto& ctx = turi::get_distributed_context();
  auto worker_ret = ctx.distributed_call(distributed_labelprop_worker_impl, args)[0];

  double average_l2_delta = worker_ret.at("average_l2_delta");
  size_t num_iterations = worker_ret.at("num_iterations");

  flexible_type label_field = variant_get_value<flexible_type>(args.at("label_field"));
  double threshold = variant_get_value<double>(args.at("threshold"));
  double self_weight = variant_get_value<double>(args.at("self_weight"));
  bool undirected = variant_get_value<int>(args.at("undirected"));
  flexible_type weight_field = variant_get_value<flexible_type>(args.at("weight_field"));

  variant_map_type ret;
  auto ret_g = std::make_shared<unity_sgraph>();
  ret_g->load_graph(output_path);
  ret["graph"] = to_variant(ret_g);
  ret["labels"] = to_variant(ret_g->get_vertices());
  ret["delta"] = average_l2_delta;
  ret["training_time"] = mytimer.current_time();
  ret["num_iterations"] = num_iterations;
  ret["self_weight"] = self_weight;
  ret["weight_field"] = weight_field;
  ret["undirected"] = undirected;
  ret["label_field"] = label_field;
  ret["threshold"] = threshold;
  auto model = std::make_shared<simple_model>(ret);

  return to_variant(model);
}

} // turi namespace

REGISTER_DML_FUNCTION(distributed_labelprop, turi::distributed_labelprop_impl);
