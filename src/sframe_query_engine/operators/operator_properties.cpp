/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <sframe_query_engine/operators/operator_properties.hpp> 
#include <sframe_query_engine/operators/operator_properties.hpp>
#include <sframe_query_engine/operators/all_operators.hpp> 
#include <sframe_query_engine/planning/planner_node.hpp>
#include <sframe_query_engine/query_engine_lock.hpp>
#include <dot_graph_printer/dot_graph.hpp>
#include <logger/assertions.hpp>

namespace turi {
namespace query_eval {

template <template <planner_node_type PType> class FieldExtractionVisitor,
          class RetType, class... CallArgs> 
RetType extract_field(planner_node_type ptype, CallArgs... call_args) {

  switch(ptype) {
    case planner_node_type::CONSTANT_NODE:
      return FieldExtractionVisitor<planner_node_type::CONSTANT_NODE>::get(call_args...);
    case planner_node_type::APPEND_NODE:
      return FieldExtractionVisitor<planner_node_type::APPEND_NODE>::get(call_args...);
    case planner_node_type::BINARY_TRANSFORM_NODE:
      return FieldExtractionVisitor<planner_node_type::BINARY_TRANSFORM_NODE>::get(call_args...);
    case planner_node_type::LOGICAL_FILTER_NODE:
      return FieldExtractionVisitor<planner_node_type::LOGICAL_FILTER_NODE>::get(call_args...);
    case planner_node_type::PROJECT_NODE:
      return FieldExtractionVisitor<planner_node_type::PROJECT_NODE>::get(call_args...);
    case planner_node_type::RANGE_NODE:
      return FieldExtractionVisitor<planner_node_type::RANGE_NODE>::get(call_args...);
    case planner_node_type::SARRAY_SOURCE_NODE:
      return FieldExtractionVisitor<planner_node_type::SARRAY_SOURCE_NODE>::get(call_args...);
    case planner_node_type::SFRAME_SOURCE_NODE:
      return FieldExtractionVisitor<planner_node_type::SFRAME_SOURCE_NODE>::get(call_args...);
    case planner_node_type::TRANSFORM_NODE:
      return FieldExtractionVisitor<planner_node_type::TRANSFORM_NODE>::get(call_args...);
    case planner_node_type::GENERALIZED_TRANSFORM_NODE:
      return FieldExtractionVisitor<planner_node_type::GENERALIZED_TRANSFORM_NODE>::get(call_args...);
#ifdef TC_HAS_PYTHON
    case planner_node_type::LAMBDA_TRANSFORM_NODE:
      return FieldExtractionVisitor<planner_node_type::LAMBDA_TRANSFORM_NODE>::get(call_args...);
#endif
    case planner_node_type::UNION_NODE:
      return FieldExtractionVisitor<planner_node_type::UNION_NODE>::get(call_args...);
    case planner_node_type::REDUCE_NODE:
      return FieldExtractionVisitor<planner_node_type::REDUCE_NODE>::get(call_args...);
    case planner_node_type::GENERALIZED_UNION_PROJECT_NODE:
      return FieldExtractionVisitor<planner_node_type::GENERALIZED_UNION_PROJECT_NODE>::get(call_args...);
    case planner_node_type::TERNARY_OPERATOR:
      return FieldExtractionVisitor<planner_node_type::TERNARY_OPERATOR>::get(call_args...);
    case planner_node_type::IDENTITY_NODE:
      return FieldExtractionVisitor<planner_node_type::IDENTITY_NODE>::get(call_args...);
    case planner_node_type::INVALID:
      ASSERT_MSG(false, "Infering type of an invalid node");
      return RetType();
  }

  return RetType(); 
}

////////////////////////////////////////////////////////////////////////////////

template <planner_node_type PType> struct visitor_infer_type {
  static std::vector<flex_type_enum> get(pnode_ptr p) {
    return operator_impl<PType>::infer_type(p);
  }
};

std::vector<flex_type_enum> infer_planner_node_type(pnode_ptr pnode) {
  std::lock_guard<recursive_mutex> GLOBAL_LOCK(global_query_lock);

  if (pnode->any_operator_parameters.count("__type_memo__")) {
    return pnode->any_operator_parameters["__type_memo__"].as<std::vector<flex_type_enum>>();
  }

  std::vector<flex_type_enum> retval
      = extract_field<visitor_infer_type, std::vector<flex_type_enum> >(pnode->operator_type, pnode); 

  if(!retval.empty())
    pnode->any_operator_parameters["__type_memo__"] = retval;
  
  return retval;
}; 

////////////////////////////////////////////////////////////////////////////////

template <planner_node_type PType> struct visitor_infer_length { 
  static int64_t get(pnode_ptr p) {
    return operator_impl<PType>::infer_length(p);
  }
};

int64_t infer_planner_node_length(pnode_ptr pnode) {
  std::lock_guard<recursive_mutex> GLOBAL_LOCK(global_query_lock);
  
  if (pnode->any_operator_parameters.count("__length_memo__")) {
    return pnode->any_operator_parameters["__length_memo__"].as<int64_t>();
  }

  int64_t retval = extract_field<visitor_infer_length, int64_t>(pnode->operator_type, pnode);

  if (retval != -1)
    pnode->any_operator_parameters["__length_memo__"] = retval;
  
  return retval;
}

////////////////////////////////////////////////////////////////////////////////

size_t infer_planner_node_num_output_columns(pnode_ptr pnode) {
  return infer_planner_node_type(pnode).size();
}

////////////////////////////////////////////////////////////////////////////////

static void _fill_dependency_set(pnode_ptr tip, std::set<pnode_ptr>& seen_nodes) {

  if(!seen_nodes.count(tip)) {
    seen_nodes.insert(tip); 

    for(pnode_ptr input : tip->inputs) {
      _fill_dependency_set(input, seen_nodes);
    }
  }
}

/** Returns the number of nodes in this planning graph, including pnode. 
 */
size_t infer_planner_node_num_dependency_nodes(std::shared_ptr<planner_node> pnode) {
  std::lock_guard<recursive_mutex> GLOBAL_LOCK(global_query_lock);

  std::set<pnode_ptr> seen_node_memo;
  _fill_dependency_set(pnode, seen_node_memo);
  return seen_node_memo.size();
}



////////////////////////////////////////////////////////////////////////////////

template <planner_node_type PType> struct visitor_planner_to_operator { 
  static std::shared_ptr<query_operator> get(pnode_ptr p) {
    return operator_impl<PType>::from_planner_node(p);
  }
};

std::shared_ptr<query_operator> planner_node_to_operator(pnode_ptr pnode) {
  
  return extract_field<visitor_planner_to_operator,
      std::shared_ptr<query_operator> >(pnode->operator_type, pnode);
}

////////////////////////////////////////////////////////////////////////////////

template <planner_node_type PType> struct visitor_get_name { 
  static std::string get() {
    return operator_impl<PType>::name();
  }
};

/**  Get the name of the node from the type.
 */
std::string planner_node_type_to_name(planner_node_type ptype) {
  return  extract_field<visitor_get_name, std::string>(ptype);
}

////////////////////////////////////////////////////////////////////////////////

/**  Get the type of the node from the name. 
 */
planner_node_type planner_node_name_to_type(const std::string& name) { 

  static std::map<std::string, planner_node_type> name_to_type_map;

  if(name_to_type_map.empty()) {
    for(int i = 0; i < int(planner_node_type::INVALID); ++i) {
      planner_node_type t = static_cast<planner_node_type>(i);
      name_to_type_map[planner_node_type_to_name(t)] = t;
    }
  }

  auto it = name_to_type_map.find(name);

  if(it == name_to_type_map.end()) {
    ASSERT_MSG(false, (std::string("Operator name ") + name + " not found.").c_str());
  }

  return it->second; 
}

////////////////////////////////////////////////////////////////////////////////

template <planner_node_type PType> struct visitor_get_attributes { 
  static query_operator_attributes get() {
    return operator_impl<PType>::attributes();
  }
};

/**  Get the name of the node from the type.
 */
query_operator_attributes planner_node_type_to_attributes(planner_node_type ptype) {
  return  extract_field<visitor_get_attributes, query_operator_attributes>(ptype);
}

////////////////////////////////////////////////////////////////////////////////

/**  This operator consumes all inputs at the same rate, and there
 *  is exactly one row for every input row.
 */
bool consumes_inputs_at_same_rates(const query_operator_attributes& attributes) {
  return (attributes.num_inputs == 1
          || (attributes.attribute_bitfield & query_operator_attributes::LINEAR)
          || (attributes.attribute_bitfield & query_operator_attributes::SUB_LINEAR));
}

bool consumes_inputs_at_same_rates(const pnode_ptr& n) {
  return consumes_inputs_at_same_rates(planner_node_type_to_attributes(n->operator_type));
}

////////////////////////////////////////////////////////////////////////////////

/**  A collection of flags used in actually doing the query
 *   optimization.
 */
bool is_linear_transform(const query_operator_attributes& attributes) {
  return (consumes_inputs_at_same_rates(attributes)
          && !is_source_node(attributes)
          && (attributes.attribute_bitfield & query_operator_attributes::LINEAR));
}

bool is_linear_transform(const pnode_ptr& n) {
  return is_linear_transform(planner_node_type_to_attributes(n->operator_type));
}

////////////////////////////////////////////////////////////////////////////////

/**  This operator consumes all inputs at the same rate, but reduces
 *   the rows in the output.
 */
bool is_sublinear_transform(const query_operator_attributes& attributes) {
  return (consumes_inputs_at_same_rates(attributes)
          && !is_source_node(attributes)
          && (attributes.attribute_bitfield & query_operator_attributes::SUB_LINEAR));
}

bool is_sublinear_transform(const pnode_ptr& n) {
  return is_sublinear_transform(planner_node_type_to_attributes(n->operator_type));
}

////////////////////////////////////////////////////////////////////////////////

/**  This operator is a source node.
 */
bool is_source_node(const query_operator_attributes& attributes) {
  return attributes.attribute_bitfield & query_operator_attributes::SOURCE;
}

/**  This operator is a source node.
 */
bool is_source_node(const pnode_ptr& n) {
  return is_source_node(planner_node_type_to_attributes(n->operator_type));
}

////////////////////////////////////////////////////////////////////////////////

static size_t _propagate_parallel_slicing(
    const pnode_ptr& n, std::map<pnode_ptr, size_t>& visited, size_t& counter) {

  auto it = visited.find(n);
  if(it != visited.end())
    return it->second; 
  
  if(is_source_node(n)) {
    return 1;
  }
  
  bool linear = is_linear_transform(n);
  bool sublinear = is_sublinear_transform(n);
  if(linear || sublinear) {
    ASSERT_FALSE(n->inputs.empty());
    
    size_t input_consumption = _propagate_parallel_slicing(n->inputs.front(), visited, counter);
    if(input_consumption == size_t(-1))
      return size_t(-1); 

    for(size_t i = 1; i < n->inputs.size(); ++i) {
      size_t i_c = _propagate_parallel_slicing(n->inputs[i], visited, counter);
      if(i_c == size_t(-1) || i_c != input_consumption) {
        return size_t(-1);
      }
    }

    if(sublinear) {
      // A new value, as this does not preserve the ability to do parallel slicing.
      ++counter;
      visited[n] = counter;
      return counter;
    } else {
      return input_consumption;
    }
  }


  // This node isn't something we know about. 
  return size_t(-1); 
}

/** Returns true if the output of this node can be parallel sliceable
 *  by the sources on this block, and false otherwise. 
 */
bool is_parallel_slicable(const pnode_ptr& n) {
  std::map<pnode_ptr, size_t> memoizer;
  size_t counter = 1;

  return _propagate_parallel_slicing(n, memoizer, counter) != size_t(-1); 
}

/** Returns a set of integers giving the different parallel slicable
 *  units for the inputs of a particular node. If 
 */
std::vector<size_t> get_parallel_slicable_codes(const pnode_ptr& n) {
  std::map<pnode_ptr, size_t> memoizer;
  size_t counter = 1;

  std::vector<size_t> codes(n->inputs.size());
  for(size_t i = 0; i < codes.size(); ++i) {
    codes[i] = _propagate_parallel_slicing(n, memoizer, counter); 
  }

  return codes; 
}

bool _is_linear_graph(const pnode_ptr& n, std::map<pnode_ptr, bool>& memo) {
  if (memo.count(n)) {
    return memo[n];
  }
  bool ret = true;
  if (!is_linear_transform(n) && !is_source_node(n)) {
    ret = false;
  } else {
    for (const auto& input: n->inputs) {
      if (!is_linear_graph(input)) {
        ret = false;
        break;
      }
    }
  }
  memo[n] = ret;
  return ret;
}

bool is_linear_graph(const pnode_ptr& n) {
  std::map<pnode_ptr, bool> memo;
  bool ret = _is_linear_graph(n, memo);
  return ret;
}

/**************************************************************************/
/*                                                                        */
/*                           prove_equal_length                           */
/*                                                                        */
/**************************************************************************/

struct length_info {
  int64_t source_length; // -1 if unkown
  void* source_node; // if length is unknown this contains a unique descriptor
                     // of the length
};
/*
 * This function returns for a given node, a "length_info" attribute.
 *  - length_info.source_length: The length of the node if known. -1 if not known
 *  - source_node: If the length is unkonwn, this contains a unique descriptor
 *                 of the length. i.e. if two differet nodes have the same
 *                 descriptor, they have the same length. But if they have
 *                 different descriptors, there is nothing we can say about
 *                 their lengths.
 */
static length_info propagate_length(
    const pnode_ptr& n, std::map<pnode_ptr, length_info>& visited) {
  auto it = visited.find(n);
  if(it != visited.end()) return it->second;

  auto inferred_length = infer_planner_node_length(n);
  if (inferred_length >= 0) {
    length_info li{inferred_length, nullptr};
    visited[n] = li;
    return li;
  }

  if(is_linear_transform(n)) {
    ASSERT_FALSE(n->inputs.empty());
    auto li = propagate_length(n->inputs.front(), visited);
    visited[n] = li;
    return li;
  } else {
    return length_info{-1, n.get()};
  }
}

std::pair<bool, bool> prove_equal_length(const std::shared_ptr<planner_node>& a,
                                         const std::shared_ptr<planner_node>& b) {
  std::map<pnode_ptr, length_info> visited;
  auto la = propagate_length(a, visited);
  auto lb = propagate_length(b, visited);
  if (la.source_length != -1 &&  lb.source_length != -1) {
    return {true, la.source_length == lb.source_length};
  } else if (la.source_length == -1 && lb.source_length == -1) {
    // if the source id is the same, we have proved they have
    // the same length. Otherwise, we can't really say if they are the same
    // length. (could be two different sources of the same size)
    return {la.source_node == lb.source_node, la.source_node == lb.source_node};
  } else {
    return {false, false};
  }
}

////////////////////////////////////////////////////////////////////////////////
// Stuff to deal with naming things.

template <planner_node_type PType> struct visitor_repr {
  static std::string get(pnode_ptr p, pnode_tagger& ft) {
    return operator_impl<PType>::repr(p, ft);
  }
};

// Choose a short name for the pnode to track things.
static std::string to_name(uint64_t i) {

  // 13 is enough to hold a 64 bit int.
  char c[16];

  size_t idx = 0;

  while(idx < 16) {
    c[idx] = 'A' + char(i % 26);
    ++idx;
    if(i < 26) break;
    i /= 26;
  }

  return std::string(c, idx);
}

// A basic function to print a node.
std::string planner_node_repr(const std::shared_ptr<planner_node>& node) {

  std::map<pnode_ptr, std::string> names;
  pnode_tagger get_tag = [&](pnode_ptr p) -> std::string {
    auto it = names.find(p);
    if(it == names.end()) {
      auto s = to_name(names.size());
      names[p] = s;
      return s;
    } else {
      return it->second;
    }
  };

  return get_tag(node) + ": " + extract_field<visitor_repr, std::string>(node->operator_type, node, get_tag);
}

// Recursively print all nodes below a current one.
static dot_graph& recursive_print_impl(const std::shared_ptr<planner_node>& node,
                                       std::map<pnode_ptr, std::string>& node_name_map,
                                       dot_graph& graph) {

  pnode_tagger get_tag = [&](pnode_ptr p) -> std::string {
    auto it = node_name_map.find(p);
    if(it == node_name_map.end()) {
      auto s = to_name(node_name_map.size());
      node_name_map[p] = s;
      return s;
    } else {
      return it->second;
    }
  };

  std::string vid = std::to_string((ptrdiff_t)(node.get()));

  std::string name = (get_tag(node) + ": "
                      + extract_field<visitor_repr, std::string>(node->operator_type, node, get_tag));

  bool added = graph.add_vertex(vid, name);
  // return if the vertex has already been added
  if (!added) return graph;
  
  for (auto input : node->inputs) {
    std::string srcvid = std::to_string((ptrdiff_t)(input.get()));
    graph.add_edge(srcvid, vid);
    recursive_print_impl(input, node_name_map, graph);
  }
  return graph;
}

std::ostream& operator<<(std::ostream& out,
                         const std::shared_ptr<planner_node>& node) {

  dot_graph graph;
  std::map<pnode_ptr, std::string> name_lookup;
  recursive_print_impl(node, name_lookup, graph);
  graph.print(out);
  return out;
}

} // namespace query_eval

} // namespace turi
