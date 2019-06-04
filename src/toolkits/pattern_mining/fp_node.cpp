/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/pattern_mining/fp_node.hpp>

namespace turi {
namespace pattern_mining {

// FP-Node Implementation
 fp_node::fp_node(const size_t& id, const size_t& node_depth){
    item_id = id;
    item_count = 0;
    depth = node_depth;
    is_closed_node = false;
  }

 fp_node* fp_node::add_child(const size_t& child_id){
    // Create a new node
    auto new_node = std::make_shared<fp_node>(child_id, depth + 1);
    // Set this node as parent to new node
    new_node->parent_node = this;
    // Add new node to children
    children_nodes.push_back(new_node);
    fp_node* new_pointer = new_node.get();
    return new_pointer;
  }

  fp_node* fp_node::get_child(const size_t& child_id) const{
    for(const auto& child_node: children_nodes){
      if(child_node->item_id == child_id){
        fp_node* child_pointer = child_node.get();
        return child_pointer;
      }
    }
    // No Node exists
    return nullptr;
  }

  std::vector<size_t> fp_node::get_path_to_root() {
    std::vector<size_t> ids;
    fp_node* current_node = this;
    while(current_node->item_id != ROOT_ID){
      ids.push_back(current_node->item_id);
      current_node = current_node->parent_node;
    }
    return ids;
  }

  bool fp_node::is_closed() const {
    bool is_closed;
    // Once closed, a node will remain closed
    if(is_closed_node){
      is_closed = true;
      return is_closed;
    }

    // Check if the node became closed
    if(children_nodes.empty()){
      is_closed = true;
      return is_closed;
    } else {
      for(const auto& child_node: children_nodes){
        if(child_node->item_count == item_count){
          is_closed = false;
          return is_closed;
        }
      }
      is_closed = true;
      return is_closed;
    }
  }

  void fp_node::erase(){
    // If node has a parent
    if(parent_node){
      // Find parent's pointer to this node
      for(auto it = parent_node->children_nodes.begin(); \
          it != parent_node->children_nodes.end(); it++){
        auto candidate_node = *it;
        if(candidate_node->item_id == item_id){
          candidate_node.reset();
          parent_node->children_nodes.erase(it);
          break;
        }
      }
    }
  }


} // namespace pattern_mining
} // namespace turi
