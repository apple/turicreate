/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/graph_analytics/label_propagation.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>
#include <sframe/algorithm.hpp>
#include <table_printer/table_printer.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <Eigen/Core>
#include <export.hpp>

namespace turi {
namespace label_propagation {

  /**
   * Global variables 
   */
  const std::string LABEL_COLUMN_PREFIX = "P";
  const std::string PREDICTED_LABEL_COLUMN_NAME = "predicted_label";
  std::string label_field = "";
  double threshold = 1e-3;
  std::string weight_field = "";
  double self_weight = 1.0;
  bool undirected = false;
  bool single_precision = false;
  int max_iterations = -1;
  const int MAX_CLASSES = 1000;

  const variant_map_type& get_default_options() {
    static const variant_map_type DEFAULT_OPTIONS {
      {"threshold", 1E-3},
      {"weight_field", ""},
      {"self_weight", 1.0},
      {"undirected", false},
      {"max_iterations", -1},
    };
    return DEFAULT_OPTIONS;
  }

  /**************************************************************************/
  /*                                                                        */
  /*                   Setup and Teardown functions                         */
  /*                                                                        */
  /**************************************************************************/
  void setup(variant_map_type& params) {
    for (const auto& opt : get_default_options()) {
      params.insert(opt);  // Doesn't overwrite keys already in params
    }

    label_field = safe_varmap_get<flexible_type>(
        params, "label_field").get<std::string>();
    weight_field = safe_varmap_get<flexible_type>(
        params, "weight_field").get<std::string>();
    threshold = safe_varmap_get<flexible_type>(params, "threshold");
    self_weight = safe_varmap_get<flexible_type>(params, "self_weight");
    undirected = safe_varmap_get<flexible_type>(params, "undirected");
    max_iterations = safe_varmap_get<flexible_type>(params, "max_iterations");
    if (params.count("single_precision")) {
      single_precision =
	  safe_varmap_get<flexible_type>(params, "single_precision");
      if (single_precision) {
        logprogress_stream << "Running label propagation using single precision" << std::endl;
      }
    }
  }

  /**
   * Running label propagation on graph g until converence.
   */
  template<typename FLOAT_TYPE>
  void run(sgraph& g, size_t& num_iter, double& average_l2_delta) {

    /// Type defs
    typedef std::vector<std::vector<flexible_type>> flex_column_type;

    /**
     * Graph related constant data
     */
    size_t num_classes = 0;
    size_t num_partitions = g.get_num_partitions();

    // Vertex labels from input data
    flex_column_type vertex_labels = g.fetch_vertex_data_field_in_memory(label_field);

    std::atomic<size_t> num_labeled_vertices(0);
    size_t num_unlabeled_vertices = 0;

    /// Initialization and check vertex labels
    /// Type check:
    /// "label_field" must be integer and starts from 0
    try {
      size_t min_class = (size_t)(-1);
      size_t max_class = 0;
      typedef std::pair<size_t, size_t> min_max_pair;

      auto min_max_reducer = [&](const flexible_type& v, min_max_pair& reducer) {
        if (!v.is_na()) {
          num_labeled_vertices++;
          reducer.first = std::min<size_t>((size_t)(v), reducer.first);
          reducer.second = std::max<size_t>((size_t)(v), reducer.second);
        }
        return true;
      };
      for (size_t i = 0; i < num_partitions; ++i) {
        auto sa = g.vertex_partition(i).select_column(label_field);
        auto partial_reduce = turi::reduce<min_max_pair>(*sa, min_max_reducer,
                                                             min_max_pair{(size_t)(-1), 0});
        for (const auto& result: partial_reduce) {
          min_class = std::min<size_t>(min_class, result.first);
          max_class = std::max<size_t>(max_class, result.second);
        }
      }
      ASSERT_EQ(min_class, 0);
      num_classes = max_class + 1;
    } catch (...) {
      log_and_throw("class label must be [0, num_classes)");
    }
    logprogress_stream << "Num classes: " << num_classes << std::endl;
    if (num_classes > MAX_CLASSES)  {
      log_and_throw("Too many classes provided. Label propagation works with maximal 1000 classes.");
    }

    num_unlabeled_vertices = g.num_vertices() - num_labeled_vertices;
    logprogress_stream << "#labeled_vertices: " << (size_t)num_labeled_vertices
                       << "\t#unlabeled_vertices: " << num_unlabeled_vertices
                       << std::endl;
    if (num_unlabeled_vertices == 0)
      logprogress_stream << "Warning: all vertices are already labeled" << std::endl;


    std::vector<size_t> size_of_partition;
    for (size_t i = 0; i < num_partitions; ++i) {
      size_of_partition.push_back(g.vertex_partition(i).size());
    }

    /**
     * In memory vertex data storing the probability of each class for each vertex.
     *
     * current_label_pb[i] is a dense eigen matrix (row major ordering),
     * storing the probability for vertices in partition i.
     *
     * prev_label_pb is a store the copy of the previous iteration. 
     */
    typedef Eigen::Matrix<FLOAT_TYPE, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrix_type;
    std::unique_ptr<std::vector<matrix_type>> current_label_pb(
        new std::vector<matrix_type>(num_partitions));
    std::unique_ptr<std::vector<matrix_type>> prev_label_pb(
        new std::vector<matrix_type>(num_partitions));
    std::vector<std::vector<mutex>> vertex_locks(num_partitions);

    // Initialize class probability to zero
    for (size_t i = 0; i < num_partitions; ++i) {
      (*current_label_pb)[i] = matrix_type::Zero(size_of_partition[i], num_classes);
      (*prev_label_pb)[i] = matrix_type::Zero(size_of_partition[i], num_classes);
      vertex_locks[i].resize(size_of_partition[i]);
    }

    // Set the initial label probability
    // For labeled vertices p[k] = 1.0 if k is the given label
    // For unlabeled vertices p[k] = 1/num_classes for all k

    static double BASELINE_PROB = 1.0 / num_classes;
    for (size_t i = 0; i < num_partitions; ++i) {
      size_t num_vertices_in_partition = vertex_labels[i].size();
      parallel_for (0, num_vertices_in_partition, [&](size_t j) {
        if (!vertex_labels[i][j].is_na()) {
          size_t class_label = vertex_labels[i][j];
          if (class_label >= num_classes) {
            log_and_throw("Class label must be [0, num_classes)");
          }
          (*prev_label_pb)[i](j, class_label) = 1.0;
        } else {
          // uniform
          (*prev_label_pb)[i].row(j).setConstant(BASELINE_PROB);
        }
      });
    }

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

           FLOAT_TYPE weight = use_edge_weight ? (flex_float)scope.edge()[2] : 1.0;

           {
             auto delta = (*prev_label_pb)[source_addr.partition_id].row(source_addr.local_id) * weight;
             auto& mtx = vertex_locks[target_addr.partition_id][target_addr.local_id];
             std::lock_guard<mutex> lock(mtx);
             (*current_label_pb)[target_addr.partition_id].row(target_addr.local_id) += delta;
           }

           // Propagate from target to source 
           if (undirected) {
             auto delta = (*prev_label_pb)[target_addr.partition_id].row(target_addr.local_id) * weight;
             auto& mtx = vertex_locks[source_addr.partition_id][source_addr.local_id];
             std::lock_guard<mutex> lock(mtx);
             (*current_label_pb)[source_addr.partition_id].row(source_addr.local_id) += delta;
           }
         };

    // Done with all initializations, this is the real for loop
    table_printer table({{"Iteration", 0}, {"Average l2 change in class probability", 0}});
    table.print_header();
    size_t iter = 0;
    while (true) {
      if (max_iterations > 0 && (int)iter >= max_iterations)
        break;

      ++iter;
      if(cppipc::must_cancel()) {
        log_and_throw(std::string("Toolkit cancelled by user."));
      }

      // Stores the total l2 diff in label probability
      turi::atomic<FLOAT_TYPE> total_l2_diff = 0.0;

      // initialize with the self weight of the the previous label value
      for (size_t i = 0; i < num_partitions; ++i) {
        (*current_label_pb)[i] = (*prev_label_pb)[i] * self_weight;
      }

      // Label Propagation
      if (weight_field.empty()) {
        sgraph_compute::fast_triple_apply(g, apply_fn, {}, {});
      } else {
        sgraph_compute::fast_triple_apply(g, apply_fn, {weight_field}, {});
      }

      // Post processing:
      // 1. Normalize to probability
      // 2. Clamp labeled data
      for (size_t i = 0; i < num_partitions; ++i) {
        size_t num_vertices_in_partition = vertex_labels[i].size();
        parallel_for (0, num_vertices_in_partition, [&](size_t j) {
          if (!vertex_labels[i][j].is_na()) {
            (*current_label_pb)[i].row(j).setZero();
            size_t class_label = vertex_labels[i][j];
            (*current_label_pb)[i](j, class_label) = 1.0;
          } else {
            (*current_label_pb)[i].row(j) /= (*current_label_pb)[i].row(j).sum();
          }
        });
        auto diff = (((*current_label_pb)[i] - (*prev_label_pb)[i]).rowwise().norm().sum());
        total_l2_diff += diff;
      }

      // swap the current label and the prev label
      std::swap(current_label_pb, prev_label_pb);

      // store iteration and delta
      num_iter = iter;
      average_l2_delta =
          num_unlabeled_vertices > 0 ? total_l2_diff / num_unlabeled_vertices : 0.0;

      table.print_row(iter, average_l2_delta);

      if (average_l2_delta < threshold)
        break;
    } // end of label_propagation iterations
    table.print_footer();

    // Free some memory
    (*current_label_pb).clear();

    // Compute the predicted label by taking the argmax of each probabilty vector
    static const double EPSILON = 1e-10;
    flex_column_type predicted_labels = sgraph_compute::create_vertex_data_from_const<flexible_type>(g, 0);
    for (size_t i = 0; i < num_partitions; ++i) {
      size_t num_vertices_in_partition = vertex_labels[i].size();
      std::vector<flexible_type>& preds = predicted_labels[i];
      matrix_type& mat = (*prev_label_pb)[i];
      parallel_for(0, num_vertices_in_partition, [&](size_t rowid) {
        // Get the index of the largest value in the row, and store it in preds[rowid]
        int best_class_id = -1;
        mat.row(rowid).maxCoeff(&best_class_id);
        // If we still get uniform distribution, output NONE
        if (fabs(mat(rowid, best_class_id) - BASELINE_PROB) < EPSILON) {
          preds[rowid] = FLEX_UNDEFINED;
        } else {
          preds[rowid] = best_class_id;
        }
      });
    }
    g.add_vertex_field(predicted_labels, PREDICTED_LABEL_COLUMN_NAME, flex_type_enum::INTEGER);

    // Write the probability vector back to graph vertex data
    typedef Eigen::Map<Eigen::Matrix<double, 1, Eigen::Dynamic, Eigen::RowMajor>>
        vector_buffer_type;

    // output_columns[class_index][partition_index]
    std::vector<std::vector<std::shared_ptr<sarray<flexible_type>>>> output_columns(num_classes);
    for (auto& c : output_columns) c.resize(num_partitions);

    parallel_for(0, num_partitions, [&](size_t i) {
      size_t num_rows = (*prev_label_pb)[i].rows();

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
        mapped_buffer = ((*prev_label_pb)[i].row(j)).template cast<double>();
        for (size_t k = 0; k < num_classes; ++k) {
          *(out_iterators[k])++ = raw_buffer[k];
        }
      }

      // close sarray
      for (size_t k = 0; k < num_classes; ++k)
        output_columns[k][i]->close();
    });

    for (size_t k = 0; k < num_classes; ++k) {
      std::string column_name = LABEL_COLUMN_PREFIX + std::to_string(k);
      g.add_vertex_field(output_columns[k], column_name);
    }
    // Done
  }


  /**************************************************************************/
  /*                                                                        */
  /*                             Main Function                              */
  /*                                                                        */
  /**************************************************************************/
  variant_map_type exec(variant_map_type& params) {

    timer mytimer;
    setup(params);

    std::shared_ptr<unity_sgraph> source_graph =
        safe_varmap_get<std::shared_ptr<unity_sgraph>>(params, "graph");
    ASSERT_TRUE(source_graph != NULL);
    sgraph& source_sgraph = source_graph->get_graph();

    // Do not support vertex groups yet.
    ASSERT_EQ(source_sgraph.get_num_groups(), 1);

    // Setup the graph we are going to work on. Copying sgraph is cheap.
    sgraph g(source_sgraph);
    g.select_vertex_fields({sgraph::VID_COLUMN_NAME, label_field});
    if (weight_field.empty()) {
      g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
    } else {
      g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME, weight_field});
    }

    size_t num_iter;
    double average_l2_delta;

    if (single_precision) {
      run<float>(g, num_iter, average_l2_delta);
    } else {
      run<double>(g, num_iter, average_l2_delta);
    }

    std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));
    variant_map_type model_params;
    model_params["graph"] = to_variant(result_graph);
    model_params["labels"] = to_variant(result_graph->get_vertices());
    model_params["delta"] = average_l2_delta;
    model_params["training_time"] = mytimer.current_time();
    model_params["num_iterations"] = num_iter;
    model_params["self_weight"] = self_weight;
    model_params["weight_field"] = weight_field;
    model_params["undirected"] = undirected;
    model_params["label_field"] = label_field;
    model_params["threshold"] = threshold;

    variant_map_type response;
    response["model"]= to_variant(std::make_shared<simple_model>(model_params));
    return response;
  }

  variant_map_type get_model_fields(variant_map_type& params) {
    return {
      {"graph",
       "A new SGraph with the label probability as new vertex property"},
      {"labels", "An SFrame with label probability for each vertex"},
      {"delta", "Change of class probability in average L2 norm"},
      {"training_time", "Total training time of the model"},
      {"num_iterations", "Number of iterations"},
      {"threshold", "The convergence threshold in average L2 norm"},
      {"weight_field", "Edge weight field for weighted propagation"},
      {"self_weight", "Weight for self edge"},
      {"undirected",
       "If true, treat edge as undirected and propagate in both directions"}
    };
  }

  /**************************************************************************/
  /*                                                                        */
  /*                          Toolkit Registration                          */
  /*                                                                        */
  /**************************************************************************/
BEGIN_FUNCTION_REGISTRATION
REGISTER_NAMED_FUNCTION("create", exec, "params");
REGISTER_FUNCTION(get_model_fields, "params");
END_FUNCTION_REGISTRATION

} // end of namespace label_propagation
} // end of namespace turi
