/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/pattern_mining/fp_tree_header.hpp>

namespace turi {
namespace pattern_mining {

// Implementation of fp_tree_header

  // Constructors
  fp_tree_header::fp_tree_header() {
    headings = std::vector<fp_tree_heading>();
  }

  fp_tree_header::fp_tree_header(const std::vector<size_t>& header_ids, \
      const std::vector<size_t>& header_supports){
    if(header_ids.size() != header_supports.size()){
      log_and_throw("header_ids and header_supports must have the same size.");
    }
    for(size_t i = 0; i < header_ids.size(); i++){
      fp_tree_heading new_heading;
      new_heading.id = header_ids[i];
      new_heading.support = header_supports[i];
      new_heading.pointer = nullptr;

      headings.push_back(new_heading);
      id_index_map[new_heading.id] = i;
    }
  }

  fp_tree_header::fp_tree_header(const fp_tree_header& other_header){
    headings = other_header.headings;
    id_index_map = other_header.id_index_map;
  }

  size_t fp_tree_header::get_index(const size_t& id) const {
    auto it = id_index_map.find(id);
    if(it != id_index_map.end()){
      return it->second;
    } else {
      return 0;
    }
  }

  std::vector<std::pair<size_t,size_t>> fp_tree_header::sort_transaction( \
      const std::vector<size_t>& new_transaction) const {

    // Get index order for transaction
    std::vector<std::pair<size_t,size_t>> new_transaction_pairs;
    for(const auto& id: new_transaction){
      auto it = id_index_map.find(id);
      if(it != id_index_map.end()){
        new_transaction_pairs.push_back(*it);
      }
    }

    // Sort on index
    std::sort(new_transaction_pairs.begin(), new_transaction_pairs.end(), \
        [](const std::pair<size_t, size_t>& left, \
           const std::pair<size_t, size_t>& right){
          return left.second < right.second;
        });

    return new_transaction_pairs;
  }

  // Getters
  std::vector<size_t> fp_tree_header::get_ids(){
    std::vector<size_t> header_ids;
    for(auto& heading : headings){
      header_ids.push_back(heading.id);
    }
    return header_ids;
  }

  std::vector<size_t> fp_tree_header::get_supports(){
    std::vector<size_t> header_supports;
    for(auto& heading : headings){
      header_supports.push_back(heading.support);
    }
    return header_supports;
  }

  std::map<size_t, fp_node*> fp_tree_header::get_pointers(){
    std::map<size_t, fp_node*> header_pointers;
    for(auto& heading : headings){
      header_pointers[heading.id] = heading.pointer;
    }
    return header_pointers;
  }

  bool fp_tree_header::has_id(const size_t& id) const {
    bool has_id;
    auto it = id_index_map.find(id);
    if(it != id_index_map.end()){
      has_id = true;
      return has_id;
    } else {
      has_id = false;
      return has_id;
    }
  }

  const fp_tree_heading fp_tree_header::get_heading(const size_t& id) const {
    auto it = id_index_map.find(id);
    if(it != id_index_map.end()){
      return headings[it->second];
    } else {
      return fp_tree_heading();
    }
  }

//  void fp_tree_header::erase_head(){
//    headings.erase(headings.begin());
//  }

  // Print Function
  std::ostream& operator<<(std::ostream& out, const fp_tree_header& header){
    out << "{";
    for(const auto& heading : header.headings){
      out << heading.id << ":" << heading.support << ",";
    }
    out << "}\n";
    return out;
  }


} // namespace pattern_mining
} // namespace turi
