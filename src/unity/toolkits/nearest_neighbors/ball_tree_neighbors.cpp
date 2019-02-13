/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

// Toolkits
#include <unity/toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <logger/assertions.hpp>
#include <sframe/groupby.hpp>
#include <sframe/sframe.hpp>
#include <sframe/sarray.hpp>

// Miscellaneous
#include <timer/timer.hpp>
#include <algorithm>
#include <Eigen/SparseCore>
#include <limits>
#include <stack>
#include <table_printer/table_printer.hpp>

namespace turi {
namespace nearest_neighbors {   

#define NONE_FLAG ((size_t) -1)


/**
 * Destructor. Make sure bad things don't happen
 */
ball_tree_neighbors::~ball_tree_neighbors(){

}


/**
* Set options
*/ 
void ball_tree_neighbors::init_options(
  const std::map<std::string, flexible_type>& _options) { 

  options.create_integer_option("leaf_size",
                            "Max number of points in a leaf node of the ball tree",
                            0,
                            0,
                            std::numeric_limits<int>::max(),
                            true); 

  options.create_string_option("label",
                             "Name of the reference dataset column with row labels.",
                             "",
                             false);

  // Set options and update model state with final option values
  options.set_options(_options); 
  add_or_update_state(flexmap_to_varmap(options.current_option_values())); 
}


/**
 * Train a ball tree nearest neighbors model.
 */
void ball_tree_neighbors::train(const sframe& X,
                                const std::vector<flexible_type>& ref_labels,
                                const std::vector<dist_component_type>& composite_distance_params,
                                const std::map<std::string, flexible_type>& opts) {

  logprogress_stream << "Starting ball tree nearest neighbors model training." << std::endl;

  timer t;
  double start_time = t.current_time();

  // Validate the inputs.
  init_options(opts);
  validate_distance_components(composite_distance_params, X);

  // Create the ml_data object for the reference data.
  initialize_model_data(X, ref_labels);

  // Initialize the distance components. NOTE: this needs data to be initialized
  // first because the row slicers need the column indices to be sorted.
  initialize_distances(); 
  
  ASSERT_FALSE(composite_distances.empty()); 
  dist_component c = composite_distances[0];


  if (metadata->num_dimensions() > 100) {
    logprogress_stream << "\nWARNING: The computational advantage of the "
      << "ball tree tends to diminish as the number of variables grows. With more "
      << "than 100 variables, the ball tree may not be optimal for this dataset."
      << std::endl;
  }


  // Figure out leaf size if the user didn't set it
  size_t leaf_size = (size_t)options.value("leaf_size");

  if (leaf_size == 0) {
    leaf_size = std::max((size_t)1000, (size_t)ceil((double)num_examples / 2048)); // max tree depth of 12
    options.set_option("leaf_size", leaf_size);
  }

  size_t min_leaves = ceil((double)num_examples / leaf_size);
  tree_depth = ceil(log2(min_leaves)) + 1;
  size_t num_leaves = std::max(size_t(1), size_t(std::pow(2, tree_depth - 1)));
  size_t num_nodes = 2 * num_leaves - 1;

  if (tree_depth > 12) {
    logprogress_stream << "\nWARNING: The ball tree is very large. Consider "
      << "increasing the leaf size to create a smaller tree and improve "
      << "performance." << std::endl;
  }


  // Initialize tree and loop objects
  if (is_dense) {
    pivots.resize(num_nodes);     // pivot observations
  } else {
    pivots_sp.resize(num_nodes);
  }

  node_radii.resize(num_nodes);                      // distance from pivot to furthest node member
  std::vector<double> first_child_radius (num_nodes, 0);  // distance from the pivot to the first child observation
  std::vector<double> median_dist (num_nodes);   // median of distances to the first child point
  double middle_dist;                            // the temporary container for the median distance in a node
  membership.resize(num_examples);               // point membership in nodes
  std::vector<double> pivot_dist(num_examples);  // distance from each point to its pivot (at the lowest tree level)
  std::vector<double> first_child_dist(num_examples);  // distance from each point to the first child (at the lowest tree level)


  // Set the radius and membership to 0 to start
  for (size_t i = 0; i < num_nodes; ++i) {
    node_radii[i] = 0;
  }

  for (size_t i = 0; i < num_examples; ++i) {
    membership[i] = 0;
  }

  size_t num_variables = metadata->num_dimensions();

  // Declare loop variables
  DenseVector p(num_variables);      // dense pivot observation
  DenseVector x(num_variables);      // dense generic observation
  SparseVector x_sp(num_variables);  // sparse pivot observation
  SparseVector p_sp(num_variables);  // sparse query observation

  size_t a;                          // generic row index for a point
  size_t idx_node;                   // index of the current node
  size_t idx_node_start;             // index of the first node on a level
  size_t idx_node_end;               // index of the last node on a level
  size_t num_level_nodes;            // number of nodes on a level

  // Switch for maintaining balance in the nodes. If a point is exactly on the
  // median this toggle indicates which child node to assign it to.
  bool first_child_median_flag = true;


  // Choose the first pivot
  // NOTE: for now this will be the first row of the reference data, but this
  // should probably be chosen randomly.

  if (is_dense) {
    mld_ref.get_iterator().fill_observation(x);
    pivots[0] = x;
  } else {
    mld_ref.get_iterator().fill_observation(x_sp);
    pivots_sp[0] = x_sp;
  }


  table_printer table( {{"Tree level", 0}, {"Elapsed Time", 0} });
  table.print_header();

  // The main loop over levels of the tree
  // NOTE: the second-to-last tree level creates the leaves, so the loop should
  // end at tree_depth - 1.
  for (size_t tree_level = 0; tree_level < (tree_depth - 1); ++tree_level) {

    if (cppipc::must_cancel()) {
      log_and_throw("Toolkit cancelled by user.");
    }

    // Get the node indices for nodes on the current level
    idx_node_start = std::pow(2, tree_level) - 1;
    idx_node_end = std::pow(2, (tree_level + 1)) - 2;
    num_level_nodes = idx_node_end - idx_node_start + 1;


    // First pass over the data
    for (auto it = mld_ref.get_iterator(); !it.done(); ++it) {
      
      // Get the required data
      a = it.row_index();
      idx_node = membership[a];

      if (is_dense) {
        p = pivots[idx_node];
        it.fill_observation(x);
        pivot_dist[a] = c.distance->distance(x, p);
      
        // find the largest distance to the pivot and index of the point
        if (pivot_dist[a] >= node_radii[idx_node]) {
          node_radii[idx_node] = pivot_dist[a];
          pivots[2 * idx_node + 1] = x;
        }

      } else {  // data is not dense
        p_sp = pivots_sp[idx_node];
        it.fill_observation(x_sp);
        pivot_dist[a] = c.distance->distance(x_sp, p_sp);

        // find the largest distance to the pivot and index of the point
        if (pivot_dist[a] >= node_radii[idx_node]) {
          node_radii[idx_node] = pivot_dist[a];
          pivots_sp[2 * idx_node + 1] = x_sp;
        }
      }
    }


    // Create vector of vectors to store the first child distances contiguously
    // for each node.
    std::vector<std::vector<double>> node_dists(num_level_nodes);
    

    // Second pass over the data
    for (auto it = mld_ref.get_iterator(); !it.done(); ++it) {
      
      // Get the required data
      a = it.row_index();
      idx_node = membership[a];

      if (is_dense) {
        p = pivots[2 * idx_node + 1];
        it.fill_observation(x);

        // Find all of the distances to the first child and pick the second child
        // as the point furthest away.
        first_child_dist[a] = c.distance->distance(x, p);

        if (first_child_dist[a] >= first_child_radius[idx_node]) {
          first_child_radius[idx_node] = first_child_dist[a];
          pivots[2 * idx_node + 2] = x;
        }

      } else { // data is not dense
        p_sp = pivots_sp[2 * idx_node + 1];
        it.fill_observation(x_sp);

        // Find all of the distances to the first child and pick the second child
        // as the point furthest away.
        first_child_dist[a] = c.distance->distance(x_sp, p_sp);

        if (first_child_dist[a] >= first_child_radius[idx_node]) {
          first_child_radius[idx_node] = first_child_dist[a];
          pivots_sp[2 * idx_node + 2] = x_sp;
        }
      }

      // Keep the first child distances compiled by node for median computation
      node_dists[idx_node - idx_node_start].push_back(first_child_dist[a]);
    }


    // Find the median first child distance for each node
    for (size_t j = 0; j < num_level_nodes; ++j) {
      if (node_dists[j].size() > 1) {

        std::nth_element(node_dists[j].begin(),
                         node_dists[j].begin() + node_dists[j].size()/2,
                         node_dists[j].end());
        middle_dist = node_dists[j][node_dists[j].size()/2];

        // if there are an even number of elements get the median of the middle two
        if (node_dists[j].size() % 2 == 0) {
          std::nth_element(node_dists[j].begin(),
                           node_dists[j].begin() + node_dists[j].size()/2 - 1,
                           node_dists[j].end());
          middle_dist = (middle_dist + node_dists[j][node_dists[j].size()/2 - 1]) / 2;
        }

        median_dist[j + idx_node_start] = middle_dist;

      } else {
        // set median distance to -1 so that singletons always go to second child
        median_dist[j + idx_node_start] = -1;
      }
    }


    // Third pass over the data
    // - assign each point to a child
    // - careful about maintaining balance here
    for (size_t b = 0; b < num_examples; ++b) {
      idx_node = membership[b];
      if (first_child_dist[b] < median_dist[idx_node]) {
        membership[b] = 2 * idx_node + 1;

      } else if (first_child_dist[b] > median_dist[idx_node]) {
        membership[b] = 2 * idx_node + 2;

      } else {  // the point is exactly on the median
          if (first_child_median_flag) {
            membership[b] = 2 * idx_node + 1;
            first_child_median_flag = false;

          } else {
            membership[b] = 2 * idx_node + 2;
            first_child_median_flag = true;
          }
      }
    }


    table.print_row(tree_level, progress_time());

  } // end loop over tree levels


  // Find the radii for each of the leaf nodes
  for (auto it = mld_ref.get_iterator(); !it.done(); ++it) {
    
    // Get the required data
    a = it.row_index();
    idx_node = membership[a];

    if (is_dense) {
      // Find the largest distance to the pivot and index of that point
      p = pivots[idx_node];
      it.fill_observation(x);
      pivot_dist[a] = c.distance->distance(x, p);

    } else { // data is not dense
      // Find the largest distance to the pivot and index of that point
      p_sp = pivots_sp[idx_node];
      it.fill_observation(x_sp);
      pivot_dist[a] = c.distance->distance(x_sp, p_sp);
    }

    if (pivot_dist[a] >= node_radii[idx_node]) {
      node_radii[idx_node] = pivot_dist[a];
    }
  }

  table.print_row(tree_depth - 1, progress_time());


  
  // Group the reference data by leaf node ID

  // convert the reference labels to an SArray.
  std::shared_ptr<sarray<flexible_type>> sa_ref_labels(new sarray<flexible_type>);
  sa_ref_labels->open_for_write();
  flex_type_enum ref_label_type = reference_labels[0].get_type();
  sa_ref_labels->set_type(ref_label_type);
  turi::copy(ref_labels.begin(), ref_labels.end(), *sa_ref_labels);
  sa_ref_labels->close();

  // convert membership into a shared pointer to an sarray
  std::shared_ptr<sarray<flexible_type>> member_column(new sarray<flexible_type>);
  member_column->open_for_write();
  member_column->set_type(flex_type_enum::INTEGER);
  turi::copy(membership.begin(), membership.end(), *member_column);
  member_column->close();

  // add the membership sarray as a column to the reference data and group
  sframe sf_refs = X.add_column(sa_ref_labels, "__nearest_neighbors_ref_label");
  sf_refs = sf_refs.add_column(member_column, "__nearest_neighbors_membership");
  sf_refs = turi::group(sf_refs, "__nearest_neighbors_membership");

  // extract the grouped membership vector and remove from the dataset.
  auto member_reader = sf_refs.select_column("__nearest_neighbors_membership")->get_reader();
  std::vector<flexible_type> temp(num_examples);
  member_reader->read_rows(0, num_examples, temp);
  std::copy(temp.begin(), temp.end(), membership.begin());

  size_t idx_member_column = sf_refs.column_index("__nearest_neighbors_membership");
  sf_refs = sf_refs.remove_column(idx_member_column);
  
  // extract the map of grouped row indices from the dataset.
  auto label_reader = sf_refs.select_column("__nearest_neighbors_ref_label")->get_reader();
  std::vector<flexible_type> temp2(num_examples);
  label_reader->read_rows(0, num_examples, temp2);

  // this modifies the model's stored reference labels, *not* the vector passed to this function.
  std::copy(temp2.begin(), temp2.end(), reference_labels.begin());

  size_t idx_label_column = sf_refs.column_index("__nearest_neighbors_ref_label");
  sf_refs = sf_refs.remove_column(idx_label_column);

  
  // Re-make the ML data with the row-permuted data for storage in the model
  mld_ref = v2::ml_data(metadata);
  mld_ref.fill(sf_refs);


  add_or_update_state({ {"method", "ball_tree"},
                        {"tree_depth", tree_depth},
                        {"leaf_size", leaf_size},
                        {"training_time", t.current_time() - start_time} });
  table.print_footer();
}  // end the create function


/**
 * Make predictions using an existing ball tree nearest neighbors model. 
 * /note For each query point compute the distance to every reference point.
 */
sframe ball_tree_neighbors::query(const v2::ml_data& mld_queries,
                                  const std::vector<flexible_type>& query_labels,
                                  const size_t k, const double radius,
                                  const bool include_self_edges) const {

  timer t;

  size_t num_queries = mld_queries.size();
  size_t num_nodes = node_radii.size();


  // Construct the distance object pointer
  ASSERT_FALSE(composite_distances.empty()); 
  dist_component c = composite_distances[0];

  // Compute the actual number of nearest neighbors and construct the data
  // structures to hold candidate neighbors while reference points are searched
  size_t kstar;

  if (k == NONE_FLAG) {
    kstar = NONE_FLAG;
  } else {
    kstar = std::min(k, mld_ref.size());
  }
  
  std::vector<neighbor_candidates> topk (num_queries,
                    neighbor_candidates(-1, kstar, radius, include_self_edges));

  parallel_for(0, num_queries, [&](size_t i) {
    topk[i].set_label(i);
  });


  atomic<size_t> n_query_points = 0;

  table_printer table({ {"Query points", 0}, {"% Complete.", 0}, {"Elapsed Time", 0}});
  table.print_header();

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {

      // Find the nearest neighbors for each query point
      // ---------------------------------------------------------------------------

      size_t num_variables = metadata->num_dimensions();

      DenseVector x(num_variables);  // reference observation
      DenseVector q(num_variables);  // query observation
      SparseVector x_sp(num_variables);  // reference observation
      SparseVector q_sp(num_variables);  // query observation

      std::stack<size_t> node_stack; // nodes that still need to be checked
      size_t idx_node;               // node currently being checked
      size_t idx_query;              // index of the query point
      bool activate_node;            // indicates whether to traverse the node
      double dist_child1;            // distance to the first child pivot of an active node
      double dist_child2;            // distance to the second child pivot of an active node
      size_t idx_start, idx_end;     // indicate where the reference data starts and stops for leaf nodes
      double dist;                   // distance between a query point and a leaf reference point
      double min_dist_possible;      // minimum possible distance between a query and a node


      auto it_ref = mld_ref.get_iterator();

      // Iterate over query points
      for (auto it_query = mld_queries.get_iterator(thread_idx, num_threads);
           !it_query.done(); ++it_query) {

        if (cppipc::must_cancel()) {
          log_and_throw("Toolkit cancelled by user.");
        }

        ASSERT_TRUE(it_query.row_index() != NONE_FLAG);

        if (is_dense) {
          it_query.fill_observation(q);
        } else {
          it_query.fill_observation(q_sp);
        }

        idx_query = it_query.row_index();
        node_stack.push(0);
        min_dist_possible = 0;


        // Loop over nodes in the traversal queue
        while (!node_stack.empty()) {
          idx_node = node_stack.top();
          node_stack.pop();

          // Compute the minimum possible distance from the query to the node
          if (is_dense) {
            min_dist_possible =
                c.distance->distance(pivots[idx_node], q) - node_radii[idx_node];
          } else {
            min_dist_possible =
                c.distance->distance(pivots_sp[idx_node], q_sp) - node_radii[idx_node];
          }

          // Decide if the node needs to be processed
          activate_node = activate_query_node(kstar, radius, min_dist_possible,
                                              topk[idx_query].candidates.size(),
                                              topk[idx_query].get_max_dist());

          if (activate_node) {

            // The active node is internal
            if (idx_node < num_nodes / 2) {

              // find the closest child pivot to the query
              if (is_dense) {
                dist_child1 = c.distance->distance(q, pivots[2 * idx_node + 1]);
                dist_child2 = c.distance->distance(q, pivots[2 * idx_node + 2]);
              } else {
                dist_child1 = c.distance->distance(q_sp, pivots_sp[2 * idx_node + 1]);
                dist_child2 = c.distance->distance(q_sp, pivots_sp[2 * idx_node + 2]);
              }

              // add child nodes to the stack, closest on the top
              if (dist_child1 <= dist_child2) {
                node_stack.push(2 * idx_node + 2);
                node_stack.push(2 * idx_node + 1);
              } else {
                node_stack.push(2 * idx_node + 1);
                node_stack.push(2 * idx_node + 2);
              }

              // The active node is a leaf
            } else {

              // figure out where the leaf members are in the reference ml_data
              idx_start = NONE_FLAG;
              idx_end = NONE_FLAG;

              for (size_t i = 0; i < membership.size(); i++) {
                if ((idx_start == NONE_FLAG) && (membership[i] == idx_node)) {
                  idx_start = i;
                }

                if ((idx_start != NONE_FLAG) && (membership[i] == idx_node)) {
                  idx_end = i + 1;
                }
              }

              DASSERT_TRUE(idx_end >= idx_start);

              if ((idx_start == NONE_FLAG) || (idx_end == NONE_FLAG)) {
                continue;  // if the node is empty, move on to the next node in the stack
              }

              for (it_ref.seek(idx_start); it_ref.row_index() != idx_end; ++it_ref) {

                if (is_dense) {
                  it_ref.fill_observation(x);
                  dist = c.distance->distance(x, q);
                } else {
                  it_ref.fill_observation(x_sp);
                  dist = c.distance->distance(x_sp, q_sp);
                }

                DASSERT_TRUE(it_ref.row_index() != NONE_FLAG);

                topk[idx_query].evaluate_point(
                    std::pair<double, size_t>(dist, it_ref.row_index()));

              }  // end the loop over reference points in the leaf
            }  // end the leaf node processing
          } // end active node processing
        } // end tree traversal for a given query point

        size_t n_query_points_so_far = (++n_query_points);

        table.print_timed_progress_row( n_query_points_so_far,
                                        std::floor((4 * 100.0 * n_query_points_so_far) / num_queries) / 4.0,
                                        progress_time());

      }  // end the loop over query points
    });  

  table.print_row("Done", " ", progress_time());
  table.print_footer();

  sframe result = write_neighbors_to_sframe(topk, reference_labels, query_labels);
  return result;
}


/**
* Turi Serialization Save
*/
void ball_tree_neighbors::save_impl(turi::oarchive& oarc) const {

  variant_deep_save(state, oarc);

  std::map<std::string, variant_type> data;

  data["membership"]         = to_variant(membership);
  data["node_radii"]         = to_variant(node_radii);
  data["tree_depth"]         = to_variant(tree_depth);
  data["is_dense"]           = to_variant(is_dense);

  variant_deep_save(data, oarc);

  // Now, a few that couldn't get saved in the above map
  oarc << pivots << pivots_sp;
  oarc << options
       << mld_ref
       << composite_params
       << untranslated_cols
       << reference_labels;
}


/**
 * Turi Serialization Load
 */
void ball_tree_neighbors::load_version(turi::iarchive& iarc, size_t version) {

  ASSERT_MSG((version == 0) || (version == 1) || (version == 2),
             "This model version cannot be loaded. Please re-save your model.");

  variant_deep_load(state, iarc);

  std::map<std::string, variant_type> data;

  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(membership);
  __EXTRACT(node_radii);
  __EXTRACT(tree_depth);
  __EXTRACT(is_dense);
#undef __EXTRACT

  iarc >> pivots >> pivots_sp;
  iarc >> options;
  iarc >> mld_ref;




  metadata = mld_ref.metadata();

  // If loading from an old version, manually construct a single component that
  // assumes a single distance across all features. This is necessary because
  // the GLC v1.1 ball tree query operates on a set of distance components
  // stored in the model, rather than a distance name as in GLC v1.
  if (version == 0) {
    auto fn = function_closure_info();
    fn.native_fn_name = "_distances.";
    fn.native_fn_name += std::string(options.value("distance"));
    std::vector<std::string> features = variant_get_value<std::vector<std::string>>(state["features"]); 
    dist_component_type p = std::make_tuple(features, fn, 1.0);
    composite_params = {p};

    // set empty untranslated columns for string features.
    untranslated_cols = {};
  }

  else {
    iarc >> composite_params;
    iarc >> untranslated_cols;    
  }


  // construct the reference labels from the target column of the reference
  // ml_data.
  if (version < 2) {
    reference_labels.resize(mld_ref.size());

    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = mld_ref.get_iterator(); !it.done(); ++it) {
        reference_labels[it.row_index()] = metadata->target_indexer()->map_index_to_value(it.target_index());
      }
    });
    
    add_or_update_state({ {"num_distance_components", 1} });
  }

  else {
    iarc >> reference_labels;
  }

  initialize_distances(); 
}


/**
* Helper function to decide if a node should be activated for a query.
*/
bool ball_tree_neighbors::activate_query_node(size_t k, double radius,
                                              double min_poss_dist,
                                              size_t num_current_neighbors,
                                              double max_current_dist) const {
  bool activate = false;

  if (k == NONE_FLAG) {

    if (radius < 0) {         // neither k nor radius is defined
      activate = true;

    } else {                  // k is undefined, radius is defined
      if (min_poss_dist < radius)
        activate = true;
    }

  } else {
      // NOTE: in future versions, the default radius will be infinity. All
      // neighbors will be checked against radius, so the following conditional
      // won't be needed.
    if (radius < 0) {         // k is defined, radius is undefined
      
      // if the candidates set is empty, max_current_dist is -1.0, but
      // num_current_neighbors should be 0, so this should trigger (unless k is
      // 0). The same thing occurs when both k and radius are defined (below).
      if ((num_current_neighbors < k) || (min_poss_dist < max_current_dist))
        activate = true;
    
    } else {                  // both k and radius are defined
      if (min_poss_dist < radius)
       if ((min_poss_dist < max_current_dist) || (num_current_neighbors < k))
        activate = true;
    }
  }

  return activate;
}

}  // namespace nearest_neighbors
}  // namespace turi
