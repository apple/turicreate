/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/query_engine_lock.hpp>

#include <core/util/sys_util.hpp>

////////////////////////////////////////////////////////////////////////////////
// Test chaining transforms

namespace turi {
namespace query_eval {

void optimization_transform_registry::set_num_stages(size_t n) {
  possible_transforms.resize(n);
  for(auto& v : possible_transforms)
    v.resize(int(planner_node_type::INVALID));
}

// Register an optimization
void optimization_transform_registry::register_optimization(
    const std::vector<size_t>& stages, std::shared_ptr<opt_transform> opt) {

  for(size_t stage : stages) {

    DASSERT_LT(stage, possible_transforms.size());
    DASSERT_EQ(possible_transforms[stage].size(), int(planner_node_type::INVALID));

    bool transform_applies = false;
    for(int i = 0; i < int(planner_node_type::INVALID); ++i) {
      planner_node_type pt = static_cast<planner_node_type>(i);

      if(opt->transform_applies(pt)) {
        possible_transforms[stage][i].push_back(opt);
        transform_applies = true;
      }
    }

    ASSERT_MSG(transform_applies, (std::string("Config ERROR: transform ") + opt->description()
                                   + " does not seem to apply to any node types.").c_str());
  }
}

/** The number of distinct optimization stages in the model.
 */
size_t optimization_transform_registry::num_stages() const {
  return possible_transforms.size();
}

const std::vector<std::shared_ptr<opt_transform> >& optimization_transform_registry::get_transforms(
    size_t stage, planner_node_type t) const
{
  DASSERT_LT(stage, possible_transforms.size());
  return possible_transforms[stage][int(t)];
}

/** Create the transformation registry.
 */
static std::shared_ptr<const optimization_transform_registry> get_transform_registry() {

  static std::shared_ptr<const optimization_transform_registry> transform_registry;

  /*  Okay, it's not ready yet.  Create it; needs to be done with a
   *  lock.
   */
  if(transform_registry == nullptr) {

    static mutex creation_lock;
    std::lock_guard<mutex> creation_lock_guard(creation_lock);

    // Return if another thread beat us to this.
    if(transform_registry != nullptr)
      return transform_registry;

    optimization_transform_registry* otr = new optimization_transform_registry;

    // First, populate the transform list.
    populate_transforms(otr);

    // Now, all the work is done!
    transform_registry.reset(otr);
  }

  return transform_registry;
}

/** Optimize the graph.
 */
pnode_ptr optimization_engine::optimize_planner_graph(
    pnode_ptr tip, const materialize_options& exec_params) {

  auto transform_registry = get_transform_registry();

  // Yes, currently need to deal with this global lock thing...
  std::lock_guard<recursive_mutex> GLOBAL_LOCK(global_query_lock);

  // Run it.
  return optimization_engine(transform_registry)._run(tip, exec_params);
}

/** Use should only be through the above optimize_planner_graph
 *  static function.
 */
optimization_engine::optimization_engine(
    std::shared_ptr<const optimization_transform_registry> _transform_registry)
    : transform_registry(_transform_registry)
{
}


// Can ensure a particular node goes back on the queue to processed.
inline void optimization_engine::mark_node_as_active(cnode_info_ptr n) {
  int idx = int(n->type);
  n->_debug_check_consistency();

  DASSERT_LT(idx, stage_type_active_mask.size());

  if(stage_type_active_mask[idx])
    active_nodes.push_back(n);
}


/**  Operation: Replace a node under the assumption that the output of
 *   this node is a correct replacement for the output of the old
 *   node.
 *
 *   To use -- 1. create a new planner node, with inputs correctly
 *   taken from the known graph. 2. Call this.  It eliminates the old
 *   node from the graph, pruning all dead nodes with no output, then
 *   stitches the new node into the graph given the constraints.
 *
 */
void optimization_engine::replace_node(cnode_info_ptr _old_node, pnode_ptr new_pnode) {

  // If this doesn't do anything.
  if(_old_node->node_discarded || _old_node->pnode == new_pnode)
    return;

  // Remake the node_info.  This inserts itself into all of the old nodes.
  node_info_ptr old_node = build_node_info(_old_node->pnode);
  DASSERT_TRUE(old_node == _old_node);

  node_info_ptr rep_node = build_node_info(new_pnode);

  old_node->_debug_check_consistency();

  // The more bullet proof we make this function, the easier it is to
  // debug the transforms.  Thus make sure the new node is not a
  // parent of the old node.
#ifndef NDEBUG
  {
    std::vector<cnode_info_ptr> head_nodes = {old_node};
    std::set<const node_info*> seen;

    for(size_t i = 0; i < head_nodes.size(); ++i) {
      if(head_nodes[i]->pnode == new_pnode) {
         ASSERT_MSG(false, "Node being replaced is downstream from replacement node.");
      }
      for(const cnode_info_ptr& c : head_nodes[i]->outputs) {
        auto it = seen.lower_bound(c.get());
        if(it == seen.end() || *it != c.get()) {
          seen.insert(it, c.get());
          head_nodes.push_back(c);
        }
      }
    }
  }
#endif

  // Translate all the outputs to the correct ones.

  // This line took more than 6 hours of debugging to get right.
  rep_node->outputs.insert(rep_node->outputs.end(), old_node->outputs.begin(), old_node->outputs.end());

  // Go through the outputs of the node, making sure the inputs are
  // replaced with the old node.
  for(const auto& n_out : old_node->outputs) {
    TURI_ATTRIBUTE_UNUSED_NDEBUG bool is_present = false;
    for(size_t i = 0; i < n_out->inputs.size(); ++i) {
      if(n_out->inputs[i] == old_node) {
        is_present = true;
        // Make sure we've kept consistency.
        DASSERT_TRUE(n_out->pnode->inputs[i] == old_node->pnode);
        n_out->inputs[i] = rep_node;
        n_out->pnode->inputs[i] = rep_node->pnode;

        /* This break is critical here for bookkeeping; when there are
           multiple inputs, we need to remove only one per entry. */
        break;
      }
    }

    DASSERT_TRUE(is_present);
  }

  old_node->outputs.clear();

  // Make sure the old node
#ifndef NDEBUG
  for(const auto& n_out : rep_node->outputs) {
    bool new_is_present = false;
    for(size_t i = 0; i < n_out->inputs.size(); ++i) {
      DASSERT_TRUE(n_out->inputs[i] != old_node);
      if(n_out->inputs[i] == rep_node)
        new_is_present = true;
    }
    DASSERT_TRUE(new_is_present);
  }

#endif

  // Prune this node out of the graph
  eliminate_node_and_prune(old_node);

  // Finally, put this node and all inputs and outputs back on the
  // processing queue.
  for(cnode_info_ptr n_in : rep_node->inputs)
    mark_node_as_active(n_in);
  for(node_info_ptr n_out : rep_node->outputs)
    mark_node_as_active(n_out);
  mark_node_as_active(rep_node);
}

/** Eliminates a node, pruning all orphaned descendents.
 *
 */
void optimization_engine:: eliminate_node_and_prune(node_info_ptr n) {

  DASSERT_TRUE(n->outputs.empty());

  // Make as discarded
  n->node_discarded = true;

  // Remove this node from it's inputs.
  while(!n->inputs.empty()) {
    node_info_ptr n_in = n->inputs.back();
    n->inputs.pop_back();

    if(n_in->outputs.size() == 1) {
      if(n_in->outputs[0] == n) {
        n_in->outputs.clear();
        eliminate_node_and_prune(n_in);
      }
    } else {
      for(size_t i = 0; i < n_in->outputs.size(); ++i) {
        if(n_in->outputs[i] == n) {
          n_in->outputs.erase(n_in->outputs.begin() + i);
          break;
        }
      }
    }
  }
}

/**  Build a graph of the nodes.
 *
 */
node_info_ptr optimization_engine::build_node_info(pnode_ptr p) {

  auto it = node_lookups.find(p);

  if(it != node_lookups.end()) {
    node_info_ptr n = it->second;
    return n;
  }

  node_info_ptr ret = std::make_shared<node_info>(p);
  all_nodes.push_back(ret);

  // Cache node
  node_lookups[p] = ret;

  // Construct the graph stuff
  ret->inputs.resize(p->inputs.size());
  for(size_t i = 0; i < p->inputs.size(); ++i) {
    ret->inputs[i] = build_node_info(p->inputs[i]);
    ret->inputs[i]->outputs.push_back(ret);
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////


// The main routine to run things
pnode_ptr optimization_engine::_run(pnode_ptr ptip, const materialize_options& exec_params) {

  // Add in a proxy node as the tip.
  pnode_ptr proxy_tip = optonly_identity_operator::make_planner_node(ptip);

  // Get the actual tip here.  This one should never change.
  node_info_ptr tip = build_node_info(proxy_tip);

  // Get the stages to run
  std::vector<size_t> stages_to_run = get_stages_to_run(exec_params);

  // Run through these stages in order.
  for(size_t stage : stages_to_run) {
    run_stage(stage, tip, exec_params);
  }

  // And we are done!
  return proxy_tip->inputs[0];
}

/** Initialize a stage -- i.e. populate the active nodes, etc.
 *
 */
void optimization_engine::run_stage(size_t stage, node_info_ptr tip, const materialize_options& exec_params) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Init the variable stuff.

  stage_type_active_mask.resize(num_types());
  for(size_t i = 0; i < stage_type_active_mask.size(); ++i) {
    stage_type_active_mask[i] =
        !(transform_registry->get_transforms(
            stage, static_cast<planner_node_type>(i)).empty());
  }

  while(true) {

    bool optimization_occured = false;

    // Build up the processing queue from all the nodes, in a
    // tip-to-leaf order.
    _build_active_node_queue(tip);

    // Run, run, little hypersquirrel!  Run up and down the tree!
    while(!active_nodes.empty()) {
      cnode_info_ptr n = active_nodes.back();
      active_nodes.pop_back();

      // It's an old reference, so ignore it.
      if(n->node_discarded)
        continue;

      // Uncomment for additional (expensive) debugging info.
      // logprogress_stream << "TIP: " << tip->pnode << std::endl;
      // tip->_debug_check_consistency();
      // logprogress_stream << "NODE: " << n->pnode << std::endl;
      // n->_debug_check_consistency();

      // Now, run through all the possible optimizations
      for(const auto& tr : transform_registry->get_transforms(stage, n->type)) {
        // logprogress_stream << "Attempting transform: " << tr->description() << std::endl;
        bool tr_applied = tr->apply_transform(this, n);

        if(tr_applied) {
#ifndef NDEBUG
          logstream(LOG_INFO) << "Applied transform: " << tr->description() << std::endl;
#endif
          optimization_occured = true;

          // Uncomment for additional (expensive) debugging info.
          // logprogress_stream << tip->pnode << std::endl;
          // tip->_debug_check_consistency();

          // When a transform is applied, any relevant nodes are put
          // back on the active node queue.
          break;
        }
      }
    }

    if(!optimization_occured)
      break;
  }
}

void optimization_engine::_build_active_node_queue(const cnode_info_ptr& tip) {

  active_nodes.clear();
  active_nodes.reserve(node_lookups.size());

  // Need to be deterministic.  Do a breadth first descent from the
  // tip, then reverse it.  This way the node that is the tip is
  // processed first.
  active_nodes = {tip};

  std::set<const node_info*> seen;

  for(size_t p_idx = 0; p_idx < active_nodes.size(); ++p_idx) {
    const cnode_info_ptr& n = active_nodes[p_idx];
    for(const cnode_info_ptr& nn : n->inputs) {
      auto it = seen.lower_bound(nn.get());
      if(it == seen.end() || *it != nn.get()) {
        seen.insert(it, nn.get());
        active_nodes.push_back(nn);
      }
    }
  }

  // Only keep the nodes that fit the current active node mask.
  auto new_end_it = std::remove_if(
      active_nodes.begin(), active_nodes.end(),
      [&](const cnode_info_ptr& n) {
        return !(stage_type_active_mask[int(n->type)]);
      });
  active_nodes.resize(new_end_it - active_nodes.begin());

  // Reverse things so that we process things from the tip backwards.
  std::reverse(active_nodes.begin(), active_nodes.end());
}


void optimization_engine::release_node(const node_info_ptr& ptr) {
  ptr->inputs.clear();
  ptr->outputs.clear();
  ptr->pnode.reset();
}

optimization_engine::~optimization_engine() {
  // clear cyclic shared_ptr references
  for(auto& nodes: all_nodes) {
    release_node(nodes);
  }
  node_lookups.clear();
  all_nodes.clear();
}

}}
