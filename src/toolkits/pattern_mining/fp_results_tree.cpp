/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/pattern_mining/fp_results_tree.hpp>

namespace turi {
namespace pattern_mining {

/**************************************************************************/
/*                                                                        */
/*                       Results tree                                     */
/*                                                                        */
/**************************************************************************/

/*
 * Empty constructor.
*/
fp_results_tree::fp_results_tree(){
  std::map<size_t, size_t> empty_order_map;
  std::map<size_t, fp_node*> empty_map;

  root_node = nullptr;
  id_order_map = empty_order_map;
  hash_id_map = empty_map;
}

/*
 * Standard Constructor
*/
fp_results_tree::fp_results_tree(const std::vector<size_t>& id_order){
  root_node = std::make_shared<fp_node>(ROOT_ID, 0);
  for(size_t i = 0; i < id_order.size(); i++){
    id_order_map[id_order[i]] = i;
    hash_id_map[id_order[i]] = nullptr;
  }
}

/*
 * Copy Constructor (Shallow Copy)
 */
fp_results_tree::fp_results_tree(const fp_results_tree& other_tree){
  root_node = other_tree.root_node;
  id_order_map = other_tree.id_order_map;
  hash_id_map = other_tree.hash_id_map;
}

/**
 * Check if frequent itemset cannot be closed
 */
bool fp_results_tree::is_itemset_redundant( \
    const std::vector<size_t>& potential_itemset, \
    const size_t& support) const {
  bool is_redundant;
  // Sort potential_itemset in reverse id_order_map
  std::vector<size_t> sorted_itemset = this->sort_itemset(potential_itemset);

  // Empty itemsets are redundant (ignoring items not in id_order_map)
  if(sorted_itemset.empty()){
    is_redundant = true;
    return is_redundant;
  }

  // Get last item in potential_itemset
  size_t last_item = sorted_itemset.back();

  // Traverse the linked list
  fp_node* head_node = hash_id_map.at(last_item);
  while(head_node != nullptr){
    // Check if current itemset has a matching support
    if(head_node->item_count >= support){
      // Check if current itemset is long enough
      if(head_node->depth >= sorted_itemset.size()){
        // Check if potential_itemset is a subset of existing itemset
        bool is_subset = is_subset_on_path(sorted_itemset, head_node);
        if(is_subset){
          is_redundant = true;
          return is_redundant;
        }
      }
    }
    head_node = head_node->next_node;
  }

  // No existing superset found
  is_redundant = false;
  return is_redundant;
}

/**
 * Add a potential closed itemset to the tree
 */
void fp_results_tree::add_itemset( \
    const std::vector<size_t>& potential_itemset, \
    const size_t& support){
  DASSERT_TRUE(support > 0);
  fp_node* current_node = root_node.get();

  // Add items following the id_order
  std::vector<size_t> sorted_itemset = this->sort_itemset(potential_itemset);

  for(const size_t& id: sorted_itemset){
    // Get child node or create a new child node
    fp_node* child_node = current_node->get_child(id);
    if(child_node == nullptr){
      // Add new child node
      child_node = current_node->add_child(id);
      child_node->next_node = hash_id_map[id];
      hash_id_map[id] = child_node;
      child_node->item_count = support;
    } else if (child_node->item_count < support){
        // Update support of existing node if necessary
        child_node->item_count = support;
    }

    // Recurse
    current_node = child_node;
  }

}


/**
 * Build results tree from a collection of closed itemsets.
 */
void fp_results_tree::build_tree(const gl_sframe& closed_itemsets){
  // Check Format of closed_itemsets
  DASSERT_TRUE(closed_itemsets.num_columns() == 2);
  auto column_types = closed_itemsets.column_types();
  DASSERT_TRUE(column_types[0] == flex_type_enum::LIST);
  DASSERT_TRUE((column_types[1] == flex_type_enum::INTEGER) || \
      (column_types[1] == flex_type_enum::FLOAT));

  std::vector<size_t> potential_itemset;
  size_t support;

  // For each (itemset, support) pair
  for(const auto& row : closed_itemsets.range_iterator()){
    potential_itemset.clear();
    // Process first element (itemset)
    const flex_list& itemset = row[0].get<flex_list>();
    for(size_t i = 0; i < itemset.size(); i++){
      auto list_type = itemset[i].get_type();
      if(list_type != flex_type_enum::INTEGER && \
          list_type != flex_type_enum::FLOAT){
        log_and_throw("First column of closed_itemsets must be list of size_ts");
      }
      size_t item_id = itemset[i].to<size_t>();
      potential_itemset.push_back(item_id);
    }

    // Process second element (support)
    support = row[1].get<flex_int>();
    if(support <= 0){
      log_and_throw("Support values must be positive");
    }

    // Add itemset + element to tree
    if(potential_itemset.size() > 0){
      add_itemset(potential_itemset, support);
    } else {
      // Handle empty itemset
      root_node->item_count = support;
    }
  }
}

gl_sframe fp_results_tree::get_closed_itemsets(
    const std::shared_ptr<topk_indexer>& indexer) const {

  // Extract itemset data from results tree
  std::vector<flexible_type> ids;
  std::vector<flexible_type> supports;
  std::stack<fp_node*> fp_node_queue;
  fp_node_queue.push(root_node.get());

  while(!fp_node_queue.empty()){
    // Get current node
    auto current_node = fp_node_queue.top();
    fp_node_queue.pop();

    // Save itemset data if closed (and not root)
    if(current_node->is_closed() && (current_node->item_id != ROOT_ID)){
      // Convert vector of size_ts size_to a flexible type
      std::vector<size_t> itemset = current_node->get_path_to_root();

      flex_list flex_itemset = itemset_to_flex_list(itemset, indexer);

      ids.push_back(flex_itemset);
      supports.push_back(current_node->item_count);
    }

    // Recurse in depth first manner
    for(auto& child_node: current_node->children_nodes){
      fp_node_queue.push(child_node.get());
    }
  }

  // Construct gl_sframe
  std::map<std::string, std::vector<flexible_type>> data;
  data["pattern"] = ids;
  data["support"] = supports;
  gl_sframe closed_itemsets = gl_sframe(data);

  return closed_itemsets;
}

/**
 * Get the top-k closed itemsets.
 */
gl_sframe fp_results_tree::get_top_k_closed_itemsets(const size_t& top_k, \
    const size_t& min_length,
    const std::shared_ptr<topk_indexer>& indexer) const {
  // Setup
  std::vector<flexible_type> ids;
  std::vector<flexible_type> supports;
  std::multimap<size_t, fp_node*> itemset_queue;
  itemset_queue.emplace(0, root_node.get());

  // Extract Data
  while(!itemset_queue.empty()){
    // Pop itemset with largest support from queue
    const auto& it = std::prev(itemset_queue.end());
    auto current_node = it->second;
    itemset_queue.erase(it);
    // Save itemset data if closed and at least min_length
    if(current_node->is_closed() && (current_node->depth >= min_length)){
      // Convert vector of size_ts size_to a flexible type
      std::vector<size_t> itemset = current_node->get_path_to_root();

      flex_list flex_itemset = itemset_to_flex_list(itemset, indexer);

      ids.push_back(flex_itemset);
      supports.push_back(current_node->item_count);
    }
    // Add children to queue
    for(auto& child_node: current_node->children_nodes){
      itemset_queue.emplace(child_node->item_count, child_node.get());
    }
    // Exit on top_k itemsets found
    if(ids.size() >= top_k){
      break;
    }
  }

  // Construct gl_sframe
  std::map<std::string, std::vector<flexible_type>> data;
  data["pattern"] = ids;
  data["support"] = supports;
  gl_sframe top_k_closed_itemsets = gl_sframe(data);

  return top_k_closed_itemsets;
}

/**
 * Get the top-k closed itemsets.
 */
std::vector<dense_bitset> fp_results_tree::get_top_k_closed_bitsets(
                                            const size_t& size,
                                            const size_t& top_k,
                                            const size_t& min_length) const {
  std::vector<dense_bitset> closed_bitsets;
  std::multimap<size_t, fp_node*> itemset_queue;
  itemset_queue.emplace(0, root_node.get());

  // Extract bitsets
  while(!itemset_queue.empty()){
    // Pop itemset with largest support from queue
    const auto& it = std::prev(itemset_queue.end());
    auto current_node = it->second;
    itemset_queue.erase(it);

    // Save itemset data if closed and at least min_length
    if(current_node->is_closed() && (current_node->depth >= min_length)){
      // Convert vector of size_ts size_to a flexible type
      std::vector<size_t> itemset = current_node->get_path_to_root();

      dense_bitset bs(size);
      for(auto rit = itemset.rbegin(); rit != itemset.rend(); rit++){
        DASSERT_TRUE(*rit < size);
        bs.set_bit(*rit);
      }
      closed_bitsets.push_back(bs);
    }

    // Add children to queue
    for(auto& child_node: current_node->children_nodes){
      itemset_queue.emplace(child_node->item_count, child_node.get());
    }

    // Exit on top_k closed_bitsets found
    if(closed_bitsets.size() >= top_k){
      break;
    }
  }
  return closed_bitsets;
}

/**
 * Helper function that sorts itemset by id_order_map
 */
std::vector<size_t> fp_results_tree::sort_itemset(\
    const std::vector<size_t>& itemset) const {

  // Get Ordering Indexes
  std::vector<std::pair<size_t, size_t>> itemset_order_pair;
  for(const auto& id: itemset){
    auto it = id_order_map.find(id);
    if(it != id_order_map.end()){
      const size_t& order = it->second;
      itemset_order_pair.push_back(std::make_pair(id, order));
    }
  }

  // Sort
  std::sort(itemset_order_pair.begin(), itemset_order_pair.end(), \
        [](const std::pair<size_t,size_t>& left, \
           const std::pair<size_t, size_t>& right){
          return left.second < right.second;
        });

  // Return sorted item ids
  std::vector<size_t> sorted_itemset;
  for(const auto& id_order_pair: itemset_order_pair){
    sorted_itemset.push_back(id_order_pair.first);
  }

  return sorted_itemset;

}

/**
 * Get support of a sorted itemset
 */
size_t fp_results_tree::get_support(const std::vector<size_t>& sorted_itemset, \
    const size_t& lower_bound_on_support) const {
  size_t support = lower_bound_on_support;
  // Handle Empty Set
  if(sorted_itemset.size() == 0){
    support = std::max(support, root_node->item_count);
    return support;
  }

  // Follow linked list of last node
  const size_t& last_item = sorted_itemset.back();

  fp_node* head_node = hash_id_map.at(last_item);
  while(head_node != nullptr){
    // Check if current itemset has a matching support
    if(head_node->item_count > support){
      // Check if current itemset is long enough
      if(head_node->depth >= sorted_itemset.size()){
        // Check if itemset is a subset of current path
        bool is_subset = is_subset_on_path(sorted_itemset, head_node);
        if(is_subset){
          support = std::max(support, head_node->item_count);
        }
      }
    }
    head_node = head_node->next_node;
  }
  return support;
}

/**
 * Prune Tree (should be rarely called)
 */
void fp_results_tree::prune_tree(const size_t& min_support){
  std::stack<fp_node*> node_stack;
  node_stack.push(this->root_node.get());

  // Reset header to linked list
  for(auto& id_pointer: hash_id_map){
    id_pointer.second = nullptr;
  }

  // Simulate Recursion
  while(!node_stack.empty()){
    // Pop Node
    fp_node* current_node = node_stack.top();
    node_stack.pop();

    std::vector<std::shared_ptr<fp_node>> remaining_children_nodes;
    // Test children
    for(auto& child_node: current_node->children_nodes){
      if(child_node->item_count < min_support){
        // Clear nodes below min_support
        child_node.reset();
      } else {
        remaining_children_nodes.push_back(child_node);
        // Rebuild header
        child_node->next_node = hash_id_map[child_node->item_id];
        hash_id_map[child_node->item_id] = child_node.get();
        // Recurse
        node_stack.push(child_node.get());
      }
    }
    current_node->children_nodes = remaining_children_nodes;
  }
}

/**
 * Helper function to check if an itemset is on the path.
 */
bool is_subset_on_path(const std::vector<size_t>& sorted_itemset, \
    fp_node* node){
  bool is_subset;

  fp_node* current_node = node;
  auto it = sorted_itemset.rbegin();
  while(current_node->item_id != ROOT_ID){
    if( *it == current_node->item_id){
      it++;
      if(it == sorted_itemset.rend()){
        is_subset = true;
        return is_subset;
      }
    }
    current_node = current_node->parent_node;
  }
  is_subset = false;
  return is_subset;
}

/**
 * Converts an itemset (a vector of item_ids) into a flex_list for an sframe.
 *   Uses an indexer to convert from ids to item type
 */
flex_list itemset_to_flex_list(const std::vector<size_t>& itemset, \
    const std::shared_ptr<topk_indexer>& indexer){
  flex_list flex_itemset;

  for(auto it = itemset.rbegin(); it != itemset.rend(); it++){
    if(indexer == nullptr){
      flex_itemset.push_back(*it);
    } else {
      flex_itemset.push_back(indexer->inverse_lookup(*it));
    }
  }
  return flex_itemset;
}

/**
 * Print an fp_results tree.
 */
void print_fp_results_tree_helper(std::ostream& out, \
    const std::shared_ptr<fp_node>& current_node){
  size_t depth = current_node->depth;
  DASSERT_LE(depth, 15); // Shouldn't be used for deep trees

  // Print current node id + count
  if(depth == 0){
    out << "'ROOT'\n";
  } else {
    for(size_t d = 0; d < depth; d++){
      out << "  |";
    }
    out << "-'" << current_node->item_id << "':";
    out << current_node->item_count << "\n";
  }
  // Recurse
  for(const auto& child_node : current_node->children_nodes){
    print_fp_results_tree_helper(out, child_node);
  }
}

std::ostream& operator<<(std::ostream& out, const fp_results_tree& tree){
  // print id_order_map
  out << " item_id_order = {";
  for(const auto& it : tree.id_order_map){
    out << it.first << ",";
  }
  out << "}\n";
  // print tree
  print_fp_results_tree_helper(out, tree.root_node);
  return out;
}

/**************************************************************************/
/*                                                                        */
/*                       Top-K results tree                               */
/*                                                                        */
/**************************************************************************/

/*
 * Empty constructor.
*/
fp_top_k_results_tree::fp_top_k_results_tree() : fp_results_tree() {
  top_k = TOP_K_MAX;
  min_length = 1;
}

/*
 * Standard Constructor
*/
fp_top_k_results_tree::fp_top_k_results_tree(const std::vector<size_t>& id_order,\
    const size_t& k, const size_t& length) : fp_results_tree(id_order) {
  top_k = k;
  min_length = length;
}

/*
 * Copy Constructor (Shallow Copy)
 */
fp_top_k_results_tree::fp_top_k_results_tree(\
    const fp_top_k_results_tree& other_tree) : fp_results_tree(other_tree) {
  top_k = other_tree.top_k;
  min_length = other_tree.min_length;
  min_support_heap = other_tree.min_support_heap;
}

/**
 * Insert support into the min_support heap.
 */
void fp_top_k_results_tree::insert_support(const size_t& support){
  if(min_support_heap.size() < top_k){
    min_support_heap.push(support);
  }
  else if (min_support_heap.top() < support){
      min_support_heap.pop();
      min_support_heap.push(support);
  }
}

/**
 * Get a bound estimate of the min support.
 */
size_t fp_top_k_results_tree::get_min_support_bound(){
  if(min_support_heap.size() < top_k){
    return 1;
  } else {
    return min_support_heap.top();
  }
}

/**
 * Add an itemset.
 */
void fp_top_k_results_tree::add_itemset( \
    const std::vector<size_t>& potential_itemset, \
    const size_t& support){

  // Check if the new closed itemset overwrites an old itemset
  bool new_closed_set = false;

  // Add new itemset
  DASSERT_TRUE(support > 0);
  fp_node* current_node = root_node.get();
  DASSERT_TRUE(current_node != NULL);
  // Add items following the id_order
  std::vector<size_t> sorted_itemset = this->sort_itemset(potential_itemset);

  for(const size_t& id: sorted_itemset){
    // Get child node or create a new child node
    fp_node* child_node = current_node->get_child(id);
    if(child_node == nullptr){
      // Add new child node
      child_node = current_node->add_child(id);
      child_node->next_node = hash_id_map[id];
      hash_id_map[id] = child_node;
      child_node->item_count = support;
      if ((current_node->children_nodes.size() > 1) || \
          (current_node->item_id == ROOT_ID)){
        new_closed_set = true; // New branch -> new closed itemset
      } else if (current_node->item_count > support){
        new_closed_set = true; // Superset with lower support -> new closed itemset
      }
    } else if (child_node->item_count < support){
        // Update support of existing node if necessary
        child_node->item_count = support;
        new_closed_set = true; // Subset with larger support -> new closed itemset
    }
    // Recurse
    current_node = child_node;
  }

  // Keep track of top_k supports of min_length closed itemsets
  if(new_closed_set && (potential_itemset.size() >= min_length)){
    this->insert_support(support);
  }
}

// Modified get_closed_itemsets
gl_sframe fp_top_k_results_tree::get_closed_itemsets( \
    const std::shared_ptr<topk_indexer>& indexer) const {
  return fp_results_tree::get_top_k_closed_itemsets(top_k, min_length, indexer);
}



} // namespace patten_mining
} // namespace turi
