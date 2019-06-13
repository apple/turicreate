/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <timer/timer.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/logging/table_printer/table_printer.hpp>

#include <toolkits/pattern_mining/fp_growth.hpp>
#include <toolkits/pattern_mining/rule_mining.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

#include <core/util/sys_util.hpp>

namespace turi {
namespace pattern_mining {

const std::string INDEX_COLUMN = "__INTERNAL__INDEX__";
const std::string INTERNAL_COLUMN_PREFIX = "__INTERNAL__";

/**************************************************************************/
/*                                                                        */
/*                       Model creation                                   */
/*                                                                        */
/**************************************************************************/

/**
 * Create a fp_growth pattern mining model.
 */
EXPORT std::shared_ptr<fp_growth> _pattern_mining_create(
               gl_sframe data,
               std::string item,
               std::vector<std::string> features,
               size_t min_support,
               size_t max_patterns,
               size_t min_length) {

  // Construct an empty model.
  std::shared_ptr<fp_growth> model;
  model.reset(new fp_growth);
  std::map<std::string, flexible_type> opts = {
    {"min_support", min_support},
    {"max_patterns", max_patterns},
    {"min_length", min_length}
  };

  // Data preparation & main algorithm handling.
  model->validate(data, item, features);
  model->init_options(opts);
  model->set_features(features);
  model->set_item(item);
  model->train(data);

  return model;
}

/**************************************************************************/
/*                                                                        */
/*                       FP-Growth Member functions                       */
/*                                                                        */
/**************************************************************************/

/**
* Preprocess data.
*/
gl_sframe fp_growth::preprocess(const gl_sframe& data) const {

  // Apply the indexer. Drop items that didn't occur during train time.
  const auto& _indexer = this->indexer;
  gl_sarray item_sa = data[this->item];
  gl_sframe database = data[this->features];

  database[INDEX_COLUMN] = item_sa.apply([_indexer]
      (const flexible_type& x) {
            size_t out = _indexer->lookup(x);
            if (out == (size_t)-1) {
              return FLEX_UNDEFINED;
            } else {
              return flexible_type(out);
            }
      }, flex_type_enum::INTEGER);

  return database;
}

/**
 * Validate the input data for training/prediction.
 */
void fp_growth::train(const gl_sframe& data) {

  // Stack the data.
  timer t;
  double start_time = t.current_time();

  // Index the item column
  indexer.reset(new topk_indexer());
  indexer->initialize();
  gl_sarray item_sa = data[this->item];
  size_t src_size = item_sa.size();
  in_parallel([&](size_t thread_id, size_t num_threads) {
     size_t start_id = src_size * thread_id / num_threads;
     size_t end_id = src_size * (thread_id + 1) / num_threads;

     for (const flexible_type& v: item_sa.range_iterator(start_id, end_id)) {
       indexer->insert_or_update(v, thread_id);
     }
  });
  indexer->finalize();
  add_or_update_state({{"num_items", to_variant(indexer->size())}});
  logprogress_stream << "Indexing complete. Found " << indexer->size()
                     << " unique items." << std::endl;

  // Apply the indexer to the data to convert to size_t.
  gl_sframe database = data[this->features];
  const auto& _indexer = this->indexer;
  database[this->item] = item_sa.apply([_indexer]
      (const flexible_type& x) {
            return _indexer->lookup(x);
      }, flex_type_enum::INTEGER); // TO

  // Stack the data
  database = database.groupby(this->features,
           {{"pattern", aggregate::CONCAT(this->item)}});
  logprogress_stream << "Preprocessing complete. Found " << database.size()
                     << " unique transactions." << std::endl;

  // Perform Closet+
  size_t min_support = variant_get_value<size_t>(state["min_support"]);
  size_t max_patterns = variant_get_value<size_t>(state["max_patterns"]);
  size_t min_length = variant_get_value<size_t>(state["min_length"]);
  closed_itemset_tree = top_k_algorithm(database["pattern"],
                          min_support, max_patterns, min_length);

  // Extract the itemsets & bitsets from the tree.
  closed_itemset = closed_itemset_tree.get_top_k_closed_itemsets(
                              max_patterns, min_length, indexer);
  closed_bitsets = closed_itemset_tree.get_top_k_closed_bitsets(
                              indexer->size(), max_patterns, min_length);
  DASSERT_EQ(closed_bitsets.size(), closed_itemset.size());
  logprogress_stream << "Pattern mining complete. Found "
                     << closed_itemset.size() << " unique closed patterns."
                     << std::endl;

  // Save the results in state.
  add_or_update_state({{"num_examples", to_variant(data.size())}});
  add_or_update_state({{"frequent_patterns", to_variant(closed_itemset)}});
  add_or_update_state({{"num_frequent_patterns",
                                     to_variant(closed_itemset.size())}});
  add_or_update_state({{"training_time", t.current_time() - start_time}});
}


/**
 * Return true if A is a subset of B.
 */
bool is_subset(const dense_bitset& A,
               const dense_bitset& B) {
  DASSERT_EQ(A.size(), B.size());
  for (const auto& item: A) {
    if (!B.get(item)) {
      return false;
    }
  }
  return true;
}

/**
 * Extract Features
 */
gl_sframe fp_growth::extract_features(const gl_sframe& data) const {
  DASSERT_TRUE(state.count("num_items") > 0);
  size_t num_items = variant_get_value<size_t>(state.at("num_items"));
  TURI_ATTRIBUTE_UNUSED_NDEBUG size_t num_frequent_patterns =
    variant_get_value<size_t>(state.at("num_frequent_patterns"));
  DASSERT_EQ(closed_bitsets.size(), num_frequent_patterns);

  // Preprocess the dataset.
  gl_sframe ex_features = preprocess(data);

  // Stack the data
  ex_features = ex_features.groupby(this->features,
           {{"pattern", aggregate::CONCAT(INDEX_COLUMN)}});
  DASSERT_TRUE(ex_features["pattern"].dtype() == flex_type_enum::LIST);
  logprogress_stream << "Preprocessing complete. Found " << ex_features.size()
                     << " unique transactions." << std::endl;

  // Apply the extract_features function.
  gl_sarray item_sa = ex_features["pattern"];
  const auto& _closed_bitsets = this->closed_bitsets;

  ex_features["extracted_features"] = item_sa.apply(
      [_closed_bitsets, num_items]
      (const flexible_type& item_set) {

            // Convert flex_list to set.
            dense_bitset A(num_items);
            for (const auto& item: item_set.get<flex_list>()) {
              A.set_bit(item.get<flex_int>());
            }

            // Perform subset operations for each closed item set.
            flex_vec subset_vec;
            for (const auto& bs: _closed_bitsets) {
              subset_vec.push_back(flex_int(is_subset(bs, A)));
            }
            DASSERT_EQ(subset_vec.size(), _closed_bitsets.size());
            return subset_vec;
      }, flex_type_enum::VECTOR);

  ex_features.remove_column("pattern");
  return ex_features;
}

/**
 * Rule Mining
 */
gl_sframe fp_growth::predict_topk(const gl_sframe& data,
          const std::string& score_function,
          const size_t& k) const {

  TURI_ATTRIBUTE_UNUSED_NDEBUG size_t max_patterns =
    variant_get_value<size_t>(state.at("max_patterns"));
  size_t score_type = get_score_function_type_from_name(score_function);
  DASSERT_EQ(score_type, 0);
  DASSERT_LE(closed_bitsets.size(), max_patterns);

  // Preprocess the dataset.
  gl_sframe predictions = preprocess(data);

  // Stack the data
  predictions = predictions.groupby(this->features,
           {{"pattern", aggregate::CONCAT(INDEX_COLUMN)}});
  DASSERT_TRUE(predictions["pattern"].dtype() == flex_type_enum::LIST);
  logprogress_stream << "Preprocessing complete. Found " << predictions.size()
                     << " unique transactions." << std::endl;

  // Apply the rule mining function.
  gl_sarray item_sa = predictions["pattern"];
  const auto& _results_tree = this->closed_itemset_tree;
  const auto& _indexer = this->indexer;

  // TODO: Streamline this to prevent all the copies from the various types.
  predictions["prediction"] = item_sa.apply(
      [k, _indexer, _results_tree, score_type]
      (const flexible_type& item_set) {

         // Mine the set of rules.
         std::vector<size_t> _item_set;
         for(const auto& item: item_set.get<flex_list>()) {
           _item_set.push_back(item.get<flex_int>());
         }

         auto flex_rules = extract_top_k_rules(_item_set, _results_tree,
             k, score_type, _indexer);

         return flex_rules;

      }, flex_type_enum::LIST);

  // Clean up and return.
  predictions.remove_column("pattern");

  // Re-organize the output into columns.
  std::string output_column_name = INTERNAL_COLUMN_PREFIX + ".stacked_predictions";
  predictions = predictions.stack("prediction", output_column_name)
                           .unpack(output_column_name, INTERNAL_COLUMN_PREFIX,
          {flex_type_enum::LIST, flex_type_enum::LIST, flex_type_enum::FLOAT,
           flex_type_enum::INTEGER, flex_type_enum::INTEGER,
           flex_type_enum::INTEGER});

  // Rename the output columns.
  std::string lhs_name = "prefix";
  std::string rhs_name = "prediction";
  std::string score_name= "confidence";
  std::string lhs_support = "prefix support";
  std::string total_support = "joint support";
  predictions.rename(
      {{INTERNAL_COLUMN_PREFIX + ".0", lhs_name},
       {INTERNAL_COLUMN_PREFIX + ".1", rhs_name},
       {INTERNAL_COLUMN_PREFIX + ".2", score_name},
       {INTERNAL_COLUMN_PREFIX + ".3", lhs_support},
       {INTERNAL_COLUMN_PREFIX + ".5", total_support}});
  // Remove RHS support (confidence does not need it).
  predictions.remove_column(INTERNAL_COLUMN_PREFIX + ".4");
  return predictions;
}

/**
 * Get frequent patterns.
 */
gl_sframe fp_growth::get_frequent_patterns() const {
  return closed_itemset;
}

/**
 * Validate the input data for training/prediction.
 */
void fp_growth::validate(const gl_sframe& _data,
                         const std::string& item,
                         const std::vector<std::string>& features) const {

  // Check if empty.
  if (_data.size() == 0){
    log_and_throw("Input data does not contain any rows.");
  }
  if (_data.num_columns() == 0){
    log_and_throw("Input data does not contain any columns.");
  }

  // Check types.
  std::vector<std::string> cols = features;
  cols.push_back(item);
  gl_sframe data = _data.select_columns(cols);
  for (const auto& f: cols) {
    flex_type_enum dtype = data[f].dtype();
    if (dtype != flex_type_enum::INTEGER && dtype != flex_type_enum::STRING) {
      std::stringstream ss;
      ss << "Column " << f << " must be of type integer or string." << std::endl;
      log_and_throw(ss.str());
    }
  }
}

/**
 * Get a version for the object
 */
size_t fp_growth::get_version() const {
  return FP_GROWTH_VERSION;
}

/**
 * Save object using Turi's oarc
 */
void fp_growth::save_impl(oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc << options
       << closed_bitsets
       << features
       << item
       << indexer
       << closed_itemset_tree;

  const std::string & prefix = oarc.get_prefix();
  closed_itemset.save(prefix);
}

/**
 * Load the object using Turi's iarc
 */
void fp_growth::load_version(iarchive& iarc, size_t version){
  if (version > FP_GROWTH_VERSION){
    log_and_throw(
      "This model version cannot be loaded. Please re-save your model.");
  }

  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> closed_bitsets
       >> features
       >> item
       >> indexer
       >> closed_itemset_tree;

  const std::string & prefix = iarc.get_prefix();
  closed_itemset = gl_sframe(prefix);
}

/**
 * Define and set options manager options.
 */
void fp_growth::init_options(
       const std::map<std::string, flexible_type>& _options) {
  DASSERT_TRUE(options.get_option_info().size() == 0);
  options.create_integer_option(
      "min_support",
      "The minimum support to define a frequent pattern.",
      1,
      1,
      std::numeric_limits<int>::max(), false);
  options.create_integer_option(
      "max_patterns",
      "The maximum number of frequent patterns to mine.",
      100,
      1,
      std::numeric_limits<int>::max(), false);
  options.create_integer_option(
      "min_length",
      "The minimum length of each pattern to be mined.",
      1,
      1,
      std::numeric_limits<int>::max(), false);

  // Set options
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}


/**************************************************************************/
/*                                                                        */
/*                            Closet Algorithm                            */
/*                                                                        */
/**************************************************************************/
/**
 * Closet algorithm for closed item set mining.
 */
fp_results_tree closet_algorithm(const gl_sarray& database, const size_t& min_support){

  // Initialize the tree
  logprogress_stream << "Building frequent pattern tree." << std::endl;
  fp_tree my_tree = build_tree(database, min_support);
  fp_results_tree closed_itemset_tree = fp_results_tree(my_tree.header.get_ids());
  closed_itemset_tree.root_node->item_count = my_tree.root_node->item_count;

  // Recurse
  logprogress_stream << "Mining frequent pattern tree." << std::endl;
  closet_growth(my_tree, closed_itemset_tree, min_support);
  return closed_itemset_tree;
}

/**
 * Helper function for closet_algorithm. Reduces the fp_tree by mining bottom-up
 */
void closet_growth(fp_tree& my_tree, fp_results_tree& closed_itemset_tree, \
    const size_t& min_support){
  // Terminate if tree is empty
  if(my_tree.root_node->children_nodes.size() == 0){
    return;
  }

  // (Future Optimization) Prune tree if single branch.

  // For each frequent singleton (in increasing order)
  auto& headings = my_tree.header.headings;
  for(auto rit = headings.rbegin(); rit != headings.rend(); rit++){
    auto& heading = *rit;
    size_t& id = heading.id;
    size_t& support = heading.support;

    std::vector<size_t> new_prefix = my_tree.root_prefix;
    new_prefix.push_back(id);

    // Check if new_prefix could be a closed itemset
    if(!closed_itemset_tree.is_itemset_redundant(new_prefix,support)){
      // Build conditional database for new prefix
      fp_tree new_tree = my_tree.build_cond_tree(heading, min_support);

      // Recurse
      closet_growth(new_tree, closed_itemset_tree, min_support);

      // Save new_prefix if it is a closed itemset
      if(!closed_itemset_tree.is_itemset_redundant(new_prefix, support)){
        closed_itemset_tree.add_itemset(new_prefix, support);
      }
    }
  }
}

/**************************************************************************/
/*                                                                        */
/*                            Top-K Algorithm                             */
/*                                                                        */
/**************************************************************************/
/**
 * Top-K Closet Algorithm for itemset mining
 */
fp_top_k_results_tree top_k_algorithm(const gl_sarray& database, \
    size_t& min_support, const size_t& top_k, const size_t& min_length){

  // Initialize the tree
  logprogress_stream << "Building frequent pattern tree.";
  fp_top_k_tree my_tree = build_top_k_tree(database, min_support, top_k, \
      min_length);


  // Initialize results
  fp_top_k_results_tree closed_itemset_tree = \
      fp_top_k_results_tree(my_tree.header.get_ids(), top_k, min_length);
  closed_itemset_tree.root_node->item_count = my_tree.root_node->item_count;


  // Recurse
  global_top_down_growth(my_tree, closed_itemset_tree, min_support);

  // Prune Results on final min_support
  closed_itemset_tree.prune_tree(min_support);

  return closed_itemset_tree;
}

/**
 * Helper function for top_k_algorithm. Reduces the fp_tree by mining top-down.
 */
void global_top_down_growth(fp_top_k_tree& my_tree,  \
    fp_top_k_results_tree& closed_itemset_tree, size_t& min_support){
  // Terminate if tree is empty
  if(my_tree.root_node->children_nodes.size() == 0){
    return;
  }

  table_printer table({{"Iteration", 0},
                       {"Num. Patterns", 14},
                       {"Support", 10},
                       {"Current Min Support", 20},
                       {"Elapsed Time", 16}});
  table.print_header();

  // For each frequent singleton (in increasing order)
  auto& headings = my_tree.header.headings;
  size_t counter = 0;
  for(auto it = headings.begin(); it != headings.end(); it++){

    auto& heading = *it;
    size_t& id = heading.id;
    size_t& support = heading.support;
    size_t num_patterns = closed_itemset_tree.min_support_heap.size();
    table.print_row(counter, num_patterns, support, min_support, progress_time());
    counter++;

    // Check if support in transaction of min_length is large enough
    // size_t support_at_depth = my_tree.get_support(heading, my_tree.get_min_depth());
    size_t& support_at_depth = support; // Fix for predict
    if(support_at_depth >= min_support){
      std::vector<size_t> new_prefix = my_tree.root_prefix;
      new_prefix.push_back(id);

      // Check if new_prefix could be a closed itemset
      if(!closed_itemset_tree.is_itemset_redundant(new_prefix,support)){
        // Build conditional database for new prefix
        fp_top_k_tree new_tree = my_tree.build_cond_tree(heading, min_support);

        // Try to raise min_support
        size_t closed_node_bound = new_tree.get_min_support_bound();
        min_support = std::max(min_support, closed_node_bound);

        // (Future Optimization) Implement Anchor Bound
        //  size_t anchor_bound = new_tree.get_anchor_min_support_bound();
        //  min_support = std::max(min_support, anchor_bound);

        // Prune new_tree
        new_tree.prune_tree(min_support);

        // Recurse
        local_bottom_up_growth(new_tree, closed_itemset_tree, \
            min_support);

        // Save new_prefix if it is a closed itemset
        if((support >= min_support) &&
            (!closed_itemset_tree.is_itemset_redundant(new_prefix, support))) {
          closed_itemset_tree.add_itemset(new_prefix, support);

          // Try to raise min_support
          size_t current_top_k_bound = closed_itemset_tree.get_min_support_bound();
          min_support = std::max(min_support, current_top_k_bound);
        }

        // Prune original_tree on new min_support
        my_tree.prune_tree(min_support);

      }
    }
  }

  // Final row.
  table.print_row("Final",
                  closed_itemset_tree.min_support_heap.size(),
                  "-",
                  min_support,
                  progress_time());
  table.print_footer();
}

/**
 * Helper function for top_k_algorithm. Reduces the fp_tree by mining bottom-up.
 */
void local_bottom_up_growth(fp_top_k_tree& my_tree,\
    fp_top_k_results_tree& closed_itemset_tree, size_t& min_support){
  // Terminate if tree is empty
  if(my_tree.root_node->children_nodes.size() == 0){
    return;
  }

  // (Future Optimization) Prune tree if single branch.

  // For each frequent singleton (in increasing order)
  auto& headings = my_tree.header.headings;
  for(auto rit = headings.rbegin(); rit != headings.rend(); rit++){
    auto& heading = *rit;
    size_t& id = heading.id;
    size_t& support = heading.support;

    // Check if support in transaction of min_length is large enough
    // size_t support_at_depth = my_tree.get_support(heading, my_tree.get_min_depth());
    size_t& support_at_depth = support; // Fix for predict
    if(support_at_depth >= min_support || true){
      std::vector<size_t> new_prefix = my_tree.root_prefix;
      new_prefix.push_back(id);

      // Check if new_prefix could be a closed itemset
      if(!closed_itemset_tree.is_itemset_redundant(new_prefix,support)){
        // Build conditional database for new prefix
        fp_top_k_tree new_tree = my_tree.build_cond_tree(heading, min_support);

        // Try to raise min_support
        size_t closed_node_bound = new_tree.get_min_support_bound();
        min_support = std::max(min_support, closed_node_bound);
        // (Future Optimization) Implement Anchor Bound
//        size_t anchor_bound = new_tree.get_anchor_min_support_bound();
//        min_support = std::max(min_support, anchor_bound);

        // Prune new_tree
        new_tree.prune_tree(min_support);

        // Recurse
        local_bottom_up_growth(new_tree, closed_itemset_tree, \
            min_support);

        // Save new_prefix if it is a closed itemset
        if( (support >= min_support) &&
            (!closed_itemset_tree.is_itemset_redundant(new_prefix, support))){
          closed_itemset_tree.add_itemset(new_prefix, support);

          // Try to raise min_support
          size_t current_top_k_bound = closed_itemset_tree.get_min_support_bound();
          min_support = std::max(min_support, current_top_k_bound);
        }
      }
    }
  }

}


} // namespace pattern_mining
} // namespace turi
