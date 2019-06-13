/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/pattern_mining/fp_tree.hpp>
#include <core/util/basic_types.hpp>

namespace turi {
namespace pattern_mining {

  // Constructors
  // Default
  fp_tree::fp_tree(){
    root_node = nullptr;
    root_prefix = std::vector<size_t>();
    header = fp_tree_header();
  }
  // Standard Constructor
  fp_tree::fp_tree(const fp_tree_header& my_header, \
      const std::vector<size_t>& prefix){
    root_node = std::make_shared<fp_node>(ROOT_ID, 0);
    root_prefix = prefix;
    header = my_header;
  }
  // Copy Constructor (Shallow Copy)
  fp_tree::fp_tree(const fp_tree & other_tree){
    root_prefix = other_tree.root_prefix;
    root_node = other_tree.root_node;
    header = other_tree.header;
  }

  /** Destructor
   *  Erase Tree Top to Bottom one element at a time
   *  Prevents a tail recursion overflow -> segfault
   */
  fp_tree::~fp_tree(){
    // Clear root_node TODO
    root_node.reset();
//
//    for(auto& heading: header.headings){
//      while(heading.pointer != nullptr){
//        auto next_node = heading.pointer->next_node;
//        heading.pointer->next_node.reset();
//
//        // Clear current pointer
//        heading.pointer.reset();
//
//        heading.pointer = next_node;
//      }
//    }
  }

  /**
   * Delete nodes in tree with insufficient support
   */
  void fp_tree::prune_tree(const size_t& min_support) {
    // Reverse iterate over headings
    for(auto rit = header.headings.rbegin();
        rit != header.headings.rend(); rit++){
      fp_tree_heading heading = *rit;
      if(heading.support >= min_support){
        break;
      }
      else {
        // Last item_id is no longer frequent
        // Erase last item_id in header
        header.headings.pop_back();

        // Erase last item_id's linked list
        fp_node* head_node = heading.pointer;
        while(head_node != nullptr){
          auto next_node = head_node->next_node;
          head_node->erase();
          head_node = next_node;
        }
      }
    }
  }

  void fp_tree::add_transaction(const std::vector<size_t>& new_transaction, \
      const size_t& count){
    DASSERT_TRUE(count > 0);

    // Sort and get indexes of transaction from header
    auto sorted_transaction_pair = header.sort_transaction(new_transaction);

    // Update tree
    fp_node* current_node = root_node.get();
    root_node->item_count += count;
    for(const auto& id_index: sorted_transaction_pair){
      const size_t& id = id_index.first;
      const size_t& index = id_index.second;

      // Get child node or create a new child node
      fp_node* child_node = current_node->get_child(id);
      // If new node append to header list
      if(child_node == nullptr){
        child_node = current_node->add_child(id);
        child_node->next_node = header.headings[index].pointer;
        header.headings[index].pointer = child_node;
      }
      // Update Count
      child_node->item_count += count;
      // Recurse
      current_node = child_node;
    }
  }

  size_t fp_tree::get_support(const fp_tree_heading& heading, \
      const size_t& min_depth) const {
    size_t support = 0;
    // Get the pointer to the linked list for item id
    fp_node* head_node = heading.pointer;
    while(head_node != nullptr){
      if(head_node->depth >= min_depth){
        support += head_node->item_count;
      }
      head_node = head_node->next_node;
    }

    return support;
  }

  std::vector<size_t> fp_tree::get_supports_at_depth(const size_t& depth) const {
    std::vector<size_t> header_supports_at_depth;
    for(const auto& heading: header.headings){
      header_supports_at_depth.push_back(this->get_support(heading, depth));
    }
    return header_supports_at_depth;
  }

  size_t fp_tree::get_num_transactions() const {
    return root_node->item_count;
  }

  void get_descendant_supports_helper(std::shared_ptr<fp_node>& current_node, \
      std::map<size_t, size_t>& id_support_map){
    id_support_map[current_node->item_id] += current_node->item_count;
    for(auto& child_node: current_node->children_nodes){
      get_descendant_supports_helper(child_node, id_support_map);
    }
  }
  std::vector<size_t> fp_tree::get_descendant_supports( \
      fp_node* anchor_node){
    std::vector<size_t> descendant_supports;
    std::map<size_t, size_t> id_support_map;
    for(auto& child_node: anchor_node->children_nodes){
      get_descendant_supports_helper(child_node, id_support_map);
    }
    for(auto& it: id_support_map){
      descendant_supports.push_back(it.second);
    }
    return descendant_supports;
  }


  std::vector<std::pair<size_t, size_t>> fp_tree::get_cond_item_counts( \
      const fp_tree_heading& heading) const{
    // Get size of conditional fp_tree's item counts
    size_t heading_size = header.get_index(heading.id);

    // Initialize Vectors
    std::vector<size_t> cond_header_counts(heading_size);
    std::vector<size_t> cond_header_ids(heading_size);
    for(int i = 0; i < truncate_check<int64_t>(heading_size); i++){
      cond_header_ids[i] = header.headings[i].id;
    }

    // Scan the fp_tree to calculate counts
    // Get pointer to link_list associated with heading.id
    fp_node* head_node = heading.pointer;
    // For each transactions containing heading.id
    while(head_node != nullptr){
      fp_node* current_node = head_node->parent_node;
      size_t count = head_node->item_count;

      // Traverse transaction back to root
      while(current_node->item_id != ROOT_ID){
        // Add count of items in the transaction
        cond_header_counts[header.get_index(current_node->item_id)] += count;
        // Next item in transaction
        current_node = current_node->parent_node;
      }
      // Next transaction with heading id
      head_node = head_node->next_node;
    }

    // Format Output
    std::vector<std::pair<size_t, size_t>> item_counts(heading_size);
    for(int i = 0; i < truncate_check<int64_t>(heading_size); i++){
      item_counts[i] = std::make_pair(cond_header_ids[i], \
          cond_header_counts[i]);
    }

    return item_counts;
  }

  fp_tree_header build_header(const std::vector<std::pair<size_t,size_t>>& \
      item_counts, const size_t& min_support){

    // Filter item_counts on min_support
    std::vector<std::pair<size_t,size_t>> filtered_item_counts;
    for(const auto& item_count: item_counts){
      if(item_count.second >= min_support){
        filtered_item_counts.push_back(item_count);
      }
    }

    // Sort filtered_item_counts in descending order by counts
    std::sort(filtered_item_counts.begin(), filtered_item_counts.end(), \
        [](const std::pair<size_t,size_t>& left, const std::pair<size_t, size_t>& right){
          return left.second > right.second;
        });

    // Construct fp_tree_header
    std::vector<size_t> header_ids;
    std::vector<size_t> header_supports;
    for(const auto& item_freq: filtered_item_counts){
      header_ids.push_back(item_freq.first);
      header_supports.push_back(item_freq.second);
    }
    fp_tree_header header = fp_tree_header(header_ids, header_supports);

    return header;
  }

  fp_tree_header fp_tree::get_cond_header(const fp_tree_heading& heading, \
      const size_t& min_support) const {

    // Get item frequencies and ids in the conditional fp-tree
    auto item_counts = this->get_cond_item_counts(heading);

    // Sort (descending) and filter (>= min_support) ids
    fp_tree_header cond_header = build_header(item_counts, min_support);

    return cond_header;
  }

  fp_tree fp_tree::build_cond_tree(const fp_tree_heading& heading, \
      const size_t& min_support) const {

    // Get header
    auto cond_header = this->get_cond_header(heading, min_support);

    // Get prefix
    std::vector<size_t> cond_prefix = root_prefix;
    cond_prefix.push_back(heading.id);

    // Initialize condidtional tree
    fp_tree cond_tree = fp_tree(cond_header, cond_prefix);

    // Add all transactions to conditional tree

    fp_node* head_node = heading.pointer;

    // Extract and add each transaction
    while(head_node != nullptr){
      std::vector<size_t> new_transaction;
      size_t transaction_support;

      new_transaction = head_node->parent_node->get_path_to_root();
      transaction_support = head_node->item_count;

      cond_tree.add_transaction(new_transaction, transaction_support);

      head_node = head_node->next_node;

      }

    return cond_tree;
  }

  fp_tree build_tree(const gl_sarray& database, const size_t& min_support){

    // Calculate ids and frequencies for the fp_tree
    auto item_counts = get_item_counts(database);
    auto header = build_header(item_counts, min_support);

    // Initialize FP-Tree
    fp_tree global_fp_tree = fp_tree(header);

    // Insert Transactions into FP-Tree
    for(const auto& transaction_array: database.range_iterator()){

      // Convert SArray element to vector of size_ts (new_transaction)
      std::vector<size_t> new_transaction = flex_to_id_vector(transaction_array);

      // Add new_trasaction to FP-Tree
      global_fp_tree.add_transaction(new_transaction, 1);
    }
    return global_fp_tree;
  }

  std::vector<size_t> flex_to_id_vector(const flexible_type& transaction_array){
    std::set<size_t> id_set;
    if(transaction_array.get_type() != flex_type_enum::LIST) {
         log_and_throw("Only accepts SArrays of numeric lists.");
    }
    const flex_list& transaction = transaction_array.get<flex_list>();
    for(const auto& transaction_item: transaction){
      switch(transaction_item.get_type()){
        case flex_type_enum::INTEGER :
          id_set.insert(transaction_item.to<int>());
          break;

        case flex_type_enum::UNDEFINED :
          // Ignore Undefined items
          break;

        default:
          log_and_throw("Only accepts SArrays of integer lists.");
          break;
      }
    }

    std::vector<size_t> id_vector (id_set.begin(), id_set.end());
    return id_vector;
  }

  std::vector<std::pair<size_t, size_t>> get_item_counts( \
      const gl_sarray& database){

    std::map<size_t, size_t> item_frequency_map;
    // For Each Transaction
    for(const auto& transaction_array: database.range_iterator()) {
      // Get Transaction
      std::vector<size_t> new_transaction = flex_to_id_vector(transaction_array);
      // Count Each Item
      for(const size_t& item_id: new_transaction){
        item_frequency_map[item_id]++;
      }
    }

    // Format Output
    std::vector<std::pair<size_t, size_t>> item_counts;
    for(const auto& item_count: item_frequency_map){
      item_counts.push_back(item_count);
    }

    return item_counts;
  }

  // Printing Utility
  void print_tree_helper(std::ostream& out, \
      const std::shared_ptr<fp_node>& current_node){
    DASSERT_LE(current_node->depth, 15); // Shouldn't be used for deep trees
    // Print current node id + count
    if(current_node->depth == 0){
      out << "'ROOT':" << current_node->item_count << "\n";
    } else {
      for(size_t d = 0; d < current_node->depth; d++){
        out << "  |";
      }
      out << "-'" << current_node->item_id << "':";
      out << current_node->item_count << "\n";
    }
    // Recurse
    for(std::shared_ptr<fp_node> child_node : current_node->children_nodes){
      print_tree_helper(out, child_node);
    }
  }

  std::ostream& operator<<(std::ostream& out, const fp_tree& tree){
    // print header ids
    out << " header: " << tree.header;
    out << " root_prefix size: " << tree.root_prefix.size() << "\n";
    // print tree
    print_tree_helper(out, tree.root_node);
    return out;
  }

// Implementation of fp_top_k_tree
  // Default Constructor
  fp_top_k_tree::fp_top_k_tree() : fp_tree() {
    top_k = TOP_K_MAX;
    min_length = 1;
  }
  // Standard Constructor
  fp_top_k_tree::fp_top_k_tree(const fp_tree_header& header, \
      const size_t& k, const size_t& length, \
      const std::vector<size_t>& prefix) : fp_tree(header, prefix) {
    top_k = k;
    min_length = length;
  }
  // Copy Constructor (Shallow Copy)
  fp_top_k_tree::fp_top_k_tree(const fp_top_k_tree& other_tree) : \
    fp_tree(other_tree){
      top_k = other_tree.top_k;
      min_length = other_tree.min_length;
      closed_node_count = other_tree.closed_node_count;
  }

//  void fp_top_k_tree::update_closed_node_count() {
//    closed_node_count.clear();
//    // Recursively calculate closed node count
//    this->update_closed_node_count_helper(root_node.get());
//  }
//  void fp_top_k_tree::update_closed_node_count_helper(fp_node* current_node){
//    // Update support count map
//    if(current_node->depth + root_prefix.size() >= min_length){
//      if(current_node->is_closed()) {
//        current_node->is_closed_node = true;
//        closed_node_count[current_node->item_count]++;
//      }
//    }
//    // Recurse
//    for(auto& child_node: current_node->children_nodes) {
//      this->update_closed_node_count_helper(child_node.get());
//    }
//  }

  size_t fp_top_k_tree::get_min_support_bound(){
    size_t number_nodes = 0;
    size_t min_support_bound = 1; // min_support cannot be less than 1
    // Iterate over closed_node_count in decreasing support order
    for(auto rit = closed_node_count.rbegin(); rit != closed_node_count.rend();\
        rit++) {
      number_nodes += rit->second;
      if(number_nodes >= top_k){
        min_support_bound = rit->first;
        break;
      }
    }
    return min_support_bound;
  }

  fp_node* fp_top_k_tree::get_anchor_node(){
    fp_node* anchor_node = nullptr;
    // TODO Implement + Test

    return anchor_node;
  }

  // Returns the k-th largest element in vec using min-heap
  size_t get_largest(std::vector<size_t> vec, size_t k){
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> min_heap;
    // TODO Test
    for(const auto& val: vec){
      if(min_heap.size() < k){
        min_heap.push(val);
      } else if (min_heap.top() < val){
        min_heap.pop();
        min_heap.push(val);
      }
    }
  return min_heap.top();
  }

  size_t fp_top_k_tree::get_anchor_min_support_bound(){
    // TODO Test
    fp_node* anchor_node;
    std::vector<size_t> descendant_supports;
    size_t min_support_bound = 1;

    anchor_node = this->get_anchor_node();
    if(anchor_node != nullptr){
      descendant_supports = this->get_descendant_supports(anchor_node);
      min_support_bound = get_largest(descendant_supports, top_k);
    }
    return min_support_bound;
  }

  size_t fp_top_k_tree::get_min_depth() {
    if(min_length > root_prefix.size()){
      return min_length - root_prefix.size();
    } else {
      return 1;
    }
  }

  fp_top_k_tree fp_top_k_tree::build_cond_tree(const fp_tree_heading& heading, \
      size_t& min_support) const{

    // Update min_support either every top_k transactions or only 20 times
    size_t num_transactions_processed = 0;
    size_t update_frequency = std::max(top_k, this->get_num_transactions()/20);

    // Get header
    auto cond_header = this->get_cond_header(heading, min_support);

    std::vector<size_t> cond_prefix = root_prefix;
    cond_prefix.push_back(heading.id);

    // Initialize conditional tree
    fp_top_k_tree cond_tree = fp_top_k_tree(cond_header, top_k, min_length, \
        cond_prefix);

    // Add transactions to conditional tree
    fp_node* head_node = heading.pointer;

    // Extract and add each transaction
    while(head_node != nullptr){
      std::vector<size_t> new_transaction;
      new_transaction = head_node->parent_node->get_path_to_root();
      size_t transaction_support = head_node->item_count;

      cond_tree.add_transaction(new_transaction, transaction_support);


      // Raise Min Support
      num_transactions_processed += transaction_support;
      if(num_transactions_processed % update_frequency == 0){
        size_t min_support_bound = cond_tree.get_min_support_bound();
        if(min_support_bound > min_support){
          // logprogress_stream << cond_tree;
          // logprogress_stream << "New Min_Support Bound <- " << min_support_bound << std::endl;
          min_support = min_support_bound;
          cond_tree.prune_tree(min_support);
        }
      }

      head_node = head_node->next_node;
    }

    return cond_tree;
  }

  fp_top_k_tree build_top_k_tree(const gl_sarray& database, size_t& min_support,\
      const size_t& top_k, const size_t& min_length) {

    // Update min_support either every top_k transactions or only 20 times
    size_t num_transactions_processed = 0;
    size_t update_frequency = std::max(top_k, database.size()/20);

    // Calculate header for the fp_tree
    auto item_counts = get_item_counts(database);
    auto header = build_header(item_counts, min_support);

    // Initialize FP-Tree
    fp_top_k_tree global_top_k_tree = fp_top_k_tree(header, top_k, min_length);

    // Insert Transactions into FP-Tree
    for(const auto& transaction_array: database.range_iterator()){

      // Convert SArray element to vector of size_ts (new_transaction)
      std::vector<size_t> new_transaction = flex_to_id_vector(transaction_array);

      // Add new_trasaction to FP-Tree
      global_top_k_tree.add_transaction(new_transaction, 1);

      // Raise Min Support
      num_transactions_processed++;
      if(num_transactions_processed % update_frequency == 0){
        size_t min_support_bound = global_top_k_tree.get_min_support_bound();
        if(min_support_bound > min_support){
          // logprogress_stream << global_top_k_tree;
          // logprogress_stream << "New Min_Support Bound <- " << min_support_bound << std::endl;

          min_support = min_support_bound;
          global_top_k_tree.prune_tree(min_support);
        }
      }
    }
    return global_top_k_tree;
  }

  // Helper function for add_transaction
  void fp_top_k_tree::update_if_closed_node(fp_node* node, \
      const size_t& count){
    // Check if node is closed
    if(node->is_closed()){
      // Check if node is part of a transaction of at least min_length
      if(node->depth + root_prefix.size() >= min_length){
        // Check if node was already closed
        if(node->is_closed_node){
          closed_node_count[node->item_count - count]--;
          closed_node_count[node->item_count]++;
        } else {
          // Otherwise add new closed node
          closed_node_count[node->item_count]++;
        }
      }
      node->is_closed_node = true;
    }
  }

  void fp_top_k_tree::add_transaction(const std::vector<size_t>& new_transaction,\
      const size_t& count){
    DASSERT_TRUE(count > 0);

    // Sort and get indexes of transaction from header
    auto sorted_transaction_pair = header.sort_transaction(new_transaction);

    // Update tree
    fp_node* current_node = root_node.get();
    root_node->item_count += count;
    for(const auto& id_index: sorted_transaction_pair){
      const size_t& id = id_index.first;
      const size_t& index = id_index.second;

      // Get child node or create a new child node
      fp_node* child_node = current_node->get_child(id);
      // If new node append to header list
      if(child_node == nullptr){
        child_node = current_node->add_child(id);
        child_node->next_node = header.headings[index].pointer;
        header.headings[index].pointer = child_node;
      }
      // Update Count
      child_node->item_count += count;

      // Update close_node_count
      update_if_closed_node(current_node, count);

     // Recurse
      current_node = child_node;
    }

    // Check if last added node is now closed
    update_if_closed_node(current_node, count);
  }



} // namespace pattern_mining
} // namespace turi
