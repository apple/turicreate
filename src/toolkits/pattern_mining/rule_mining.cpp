/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <limits>
#include <toolkits/pattern_mining/rule_mining.hpp>

#include <core/util/basic_types.hpp>

namespace turi {
namespace pattern_mining {

/**
 * Extract the top-k rules for a given itemset
 * TODO Refactor this into subclasses + subfunctions
 */

flex_list extract_top_k_rules(const std::vector<size_t>& my_itemset, \
    const fp_results_tree& closed_itemset_tree, \
    const size_t& top_k, \
    const size_t& score_type, \
    const std::shared_ptr<topk_indexer>& indexer){
  // Process Input
  auto sorted_itemset = closed_itemset_tree.sort_itemset(my_itemset);
  const size_t num_transactions = closed_itemset_tree.get_num_transactions();
  auto score_function = get_score_function(score_type, num_transactions);

  // Setup Output
  rule_score_min_heap rule_score_heap(top_k);

  // Setup State
  std::vector<std::vector<size_t>> LHSs;
  std::vector<size_t> LHS_supports;
  std::vector<size_t> RHS;

  std::vector<size_t> empty_set;
  size_t empty_support = closed_itemset_tree.get_support(empty_set);
  LHSs.push_back(empty_set);
  LHS_supports.push_back(empty_support);
  RHS = empty_set;

  // Setup Stacks
  size_t stack_depth = 1;
  std::stack<bool> reset_stack;
  std::stack<fp_node*> node_stack;
  for(const auto& child_node: closed_itemset_tree.root_node->children_nodes){
    node_stack.push(child_node.get());
  }
  std::stack<std::vector<size_t>::iterator> iter_stack;
  iter_stack.push(sorted_itemset.begin());

  // Simulate Recursion -> While Loop
  while(!node_stack.empty()){
    fp_node* current_node = node_stack.top();
    node_stack.pop();

    // Reset State
    while(current_node->depth < stack_depth){
      stack_depth--;
      bool pop_LHS = reset_stack.top();
      reset_stack.pop();
      if(pop_LHS){
        LHSs.pop_back();
        LHS_supports.pop_back();
        iter_stack.pop();
      } else {
        RHS.pop_back();
      }
    }
    DASSERT_EQ(current_node->depth, stack_depth);

    // Check if current node is in itemset
    bool current_node_in_itemset = false;
    for(auto it = iter_stack.top(); it != sorted_itemset.end(); it++){
      if(*it == current_node->item_id){
        current_node_in_itemset = true;
        it++;
        iter_stack.push(it);
        break;
      }
    }

    // Either extend LHS or RHS
    if(current_node_in_itemset){
      // Extend LHS
      std::vector<size_t> new_LHS = LHSs.back();
      new_LHS.push_back(current_node->item_id);
      size_t new_LHS_support = closed_itemset_tree.get_support(new_LHS, \
          current_node->item_count);

      LHSs.push_back(new_LHS);
      LHS_supports.push_back(new_LHS_support);
    } else {
      // Extend RHS
      RHS.push_back(current_node->item_id);
    }

    // Check if we should add a rule
    if(current_node->is_closed() && !RHS.empty()){
      // Skip if LHS is empty when max_confidence rule
      if(score_type == MAX_CONF_SCORE && LHSs.size() == 1){
        break;
      }

      // Otherwise add new rules
      size_t RHS_support = 0;
      if(score_type != CONF_SCORE){
        RHS_support = closed_itemset_tree.get_support(RHS, \
            current_node->item_count);
      }

      rule new_rule;
      new_rule.LHS = LHSs.back();
      new_rule.LHS_support = LHS_supports.back();
      new_rule.RHS = RHS;
      new_rule.RHS_support = RHS_support;
      new_rule.total_support = current_node->item_count;

      auto new_score = score_function(new_rule);
      auto rule_score_pair = std::make_pair(new_rule, new_score);

      rule_score_heap.add_rule_score_pair(rule_score_pair);
    }

    // Recurse
    stack_depth++;
    reset_stack.push(current_node_in_itemset);
    for(const auto& child_node: current_node->children_nodes){
      node_stack.push(child_node.get());
    }

  } // End while loop

  // Set data in descending order
  auto rule_score_pairs = rule_score_heap.convert_to_sorted_vector();

  // Convert to flex_list
  flex_list flex_rules = rules_to_flex_list(rule_score_pairs, indexer);

  return flex_rules;
}


/**
 * Add rule score pair to heap
 */
void rule_score_min_heap::add_rule_score_pair( \
    const std::pair<rule, double>& rule_score_pair){
  if(min_heap.size() < top_k){
    min_heap.push(rule_score_pair);
  } else if (min_heap.top().second < rule_score_pair.second){
    min_heap.pop();
    min_heap.push(rule_score_pair);
  }
}


/**
 * Convert Heap Rule-Score-Pair to Sorted Vector of Rule-Score-Pairs
 */
std::vector<std::pair<rule, double>> rule_score_min_heap::convert_to_sorted_vector(){
  std::vector<std::pair<rule, double>> rule_score_pairs;
  while(!min_heap.empty()){
    rule_score_pairs.push_back(min_heap.top());
    min_heap.pop();
  }
  std::sort(rule_score_pairs.begin(), rule_score_pairs.end(), \
      rule_score_compare());
  return rule_score_pairs;
}


/**
 * Convert Vector of Rule-Score-Pairs to flex_list
 */
flex_list rules_to_flex_list(std::vector<std::pair<rule, double>> rule_score_pairs,
    const std::shared_ptr<topk_indexer>& indexer){
   flex_list flex_rules;
   if(!rule_score_pairs.empty()){
    for(const auto& rule_score_pair: rule_score_pairs){
      flex_list flex_rule;
      flex_rule.push_back(itemset_to_flex_list(rule_score_pair.first.LHS, indexer));
      flex_rule.push_back(itemset_to_flex_list(rule_score_pair.first.RHS, indexer));
      flex_rule.push_back(rule_score_pair.second);

      flex_rule.push_back(rule_score_pair.first.LHS_support);
      flex_rule.push_back(rule_score_pair.first.RHS_support);
      flex_rule.push_back(rule_score_pair.first.total_support);

      flex_rules.push_back(flex_rule);
    }
  } else {
    flex_list rl {FLEX_UNDEFINED, FLEX_UNDEFINED, FLEX_UNDEFINED,
      FLEX_UNDEFINED, FLEX_UNDEFINED, FLEX_UNDEFINED};
    flex_rules.push_back(rl);
  }

  return flex_rules;

}

/**
 * Extract Rules for a given itemset
 */
rule_list extract_relevant_rules(const std::vector<size_t>& my_itemset, \
    const fp_results_tree& closed_itemset_tree){
  // Sort my_itemset
  auto sorted_itemset = closed_itemset_tree.sort_itemset(my_itemset);

  rule_miner my_miner = rule_miner(sorted_itemset, closed_itemset_tree);

  // This can be parallelized
  for(auto& child_node: closed_itemset_tree.root_node->children_nodes){
    my_miner.extract_relevant_rules_helper(child_node);
  }

  rule_list my_rules = my_miner.get_rule_list();
  my_rules.num_transactions = closed_itemset_tree.get_num_transactions();

  return my_rules;
}

/**
 * Helper class for extract_relevant_rules()
 */
rule_miner::rule_miner(const std::vector<size_t>& sorted_itemset, \
    const fp_results_tree& my_results){
  closed_itemset_tree = my_results;
  std::vector<size_t> empty_set;
  size_t num_transactions = closed_itemset_tree.get_num_transactions();
  LHS_list.push_back(empty_set);
  LHS_support_list.push_back(num_transactions);
  itemset_list.push_back(sorted_itemset);
}

/**
 * Recursive helper function that builds rule_list
 */
void rule_miner::extract_relevant_rules_helper(std::shared_ptr<fp_node>& node){

  bool node_in_itemset = false;

  // Check if node is in itemset
  std::vector<size_t> remaining_itemset = itemset_list.back();
  for(auto it = remaining_itemset.begin(); it != remaining_itemset.end(); it++){
    if(*it == node->item_id){
      // If yes, add to LHS_list and LHS_support_list and shrink itemset
      node_in_itemset = true;

      std::vector<size_t> new_LHS = LHS_list.back();
      new_LHS.push_back(node->item_id);
      size_t new_LHS_support = closed_itemset_tree.get_support(new_LHS, \
          node->item_count);

      LHS_list.push_back(new_LHS);
      LHS_support_list.push_back(new_LHS_support);

      // Shorten the itemset we search in future iterations
      it++;
      remaining_itemset.erase(remaining_itemset.begin(), it);
      itemset_list.push_back(remaining_itemset);
      break;
    }
  }

  if(!node_in_itemset){
    // If node is not in itemset, add it to RHS
    RHS.push_back(node->item_id);
  }

  // Check if node represents a closed node
  if(node->is_closed() && !RHS.empty()){
    // If yes, get support for RHS
    size_t RHS_support = closed_itemset_tree.get_support(RHS, node->item_count);

    // Add a new rule for most recent LHS + RHS combination
    rule new_rule;
    new_rule.LHS = LHS_list.back();
    new_rule.LHS_support = LHS_support_list.back();
    new_rule.RHS = RHS;
    new_rule.RHS_support = RHS_support;
    new_rule.total_support = node->item_count;

    my_rules.add_rule(new_rule);
  }

  // Recurse on children nodes
  for(auto child_node: node->children_nodes){
    this->extract_relevant_rules_helper(child_node);
  }

  // Return RHS, LHS_list, LHS_support_list, and itemset_list to original state
  if(node_in_itemset){
    LHS_list.pop_back();
    LHS_support_list.pop_back();
    itemset_list.pop_back();
  } else {
    RHS.pop_back();
  }
}

/**
 * Implementation of rule_list
 */
void rule_list::append_rule_list(const rule_list& other_list){
  rules.insert(rules.end(), other_list.rules.begin(), other_list.rules.end());
}

std::vector<size_t> rule_list::get_LHS_supports() const {
  std::vector<size_t> LHS_supports;
  for(const rule& my_rule: rules){
    LHS_supports.push_back(my_rule.LHS_support);
  }
  return LHS_supports;
}

std::vector<size_t> rule_list::get_RHS_supports() const {
  std::vector<size_t> RHS_supports;
  for(const rule& my_rule: rules){
    RHS_supports.push_back(my_rule.RHS_support);
  }
  return RHS_supports;
}

std::vector<size_t> rule_list::get_total_supports() const {
  std::vector<size_t> total_supports;
  for(const rule& my_rule: rules){
    total_supports.push_back(my_rule.total_support);
  }
  return total_supports;
}

/**
 * Convert rule_list to a gl_sframe
 */
gl_sframe rule_list::to_gl_sframe(\
    const std::shared_ptr<topk_indexer>& indexer) const {
  std::vector<flexible_type> flex_LHSs;
  std::vector<flexible_type> flex_RHSs;
  std::vector<flexible_type> flex_LHS_supports;
  std::vector<flexible_type> flex_RHS_supports;
  std::vector<flexible_type> flex_total_supports;

  for(const auto& rule: rules){
    flex_LHSs.push_back(itemset_to_flex_list(rule.LHS));
    flex_RHSs.push_back(itemset_to_flex_list(rule.RHS));
    flex_LHS_supports.push_back(rule.LHS_support);
    flex_RHS_supports.push_back(rule.RHS_support);
    flex_total_supports.push_back(rule.total_support);
  }

  std::map<std::string, std::vector<flexible_type>> sf_data;
  sf_data["LHS"] = flex_LHSs;
  sf_data["RHS"] = flex_RHSs;
  sf_data["LHS_support"] = flex_LHS_supports;
  sf_data["RHS_support"] = flex_RHS_supports;
  sf_data["total_support"] = flex_total_supports;

  gl_sframe rules_sframe = gl_sframe(sf_data);

  return rules_sframe;
}


/**
 * Helper function: return the k-th largest element of a vector
 */
double get_k_largest(const std::vector<double>& scores, const size_t& top_k){
  std::priority_queue<double, std::vector<double>, std::greater<double>> min_heap;
  for(const auto& score: scores){
    if(min_heap.size() < top_k){
      min_heap.push(score);
    } else if (min_heap.top() < score){
      min_heap.pop();
      min_heap.push(score);
    }
  }

  if (min_heap.size() == 0) {
    return std::numeric_limits<double>::min();
  } else {
    return min_heap.top();
  }
}


/**
 * Extract top_k rules into a vector of flex_list
 */
flex_list rule_list::get_top_k_rules(const size_t& top_k, \
          const size_t& score_type,\
          const std::shared_ptr<topk_indexer>& indexer) const {

  std::vector<double> scores = this->score_rules(score_type);

  double min_score = get_k_largest(scores, top_k);

  // Get unordered list of top k rules + scores
  std::vector<std::pair<rule,double>> top_rule_pairs;
  for(int i = 0; i < truncate_check<int64_t>(rules.size()); i++){
    if(scores[i] >= min_score){
      std::pair<rule, double> rule_pair;
      rule_pair.first = rules[i];
      rule_pair.second = scores[i];

      top_rule_pairs.push_back(rule_pair);
    }
    if(top_rule_pairs.size() >= top_k){
      break;
    }
  }

  // Sort list in descending order
  std::sort(top_rule_pairs.begin(), top_rule_pairs.end(), rule_score_compare());

  // Format output to flex_list
  flex_list flex_rules = rules_to_flex_list(top_rule_pairs, indexer);
  return flex_rules;
}

/**
 * Helper for printing rule_list
 */
std::ostream& operator<<(std::ostream& out, const rule_list& my_rules){
  out << my_rules.to_gl_sframe();
  return out;
}



// Score functions for Rules ////////////////////////////////////
std::vector<double> rule_list::score_rules(const size_t& score_type) const{
  std::vector<double> scores;

  auto score_function = get_score_function(score_type, num_transactions);
  // This can be parallelized
  for(const auto& rule: rules){
    double score = score_function(rule);
    scores.push_back(score);
  }
  return scores;
}

/**
 * Returns the score function (rule) -> score
 */
std::function<double (const rule&)> get_score_function(const size_t& score_type,
    const size_t& num_transactions){
  std::function<double (const rule&)> score_function;
  switch(score_type){
    case CONF_SCORE:
      score_function = [](const rule& x) {
        flexible_type score = confidence_score(x.LHS_support,
                                               x.RHS_support,
                                               x.total_support);
        return score;};
      break;

    case LIFT_SCORE:
      score_function = [&num_transactions](const rule& x) {
        flexible_type score = lift_score(x.LHS_support/((double) num_transactions),
                                         x.RHS_support/((double) num_transactions),
                                         x.total_support/((double)num_transactions));
        return score;};
      break;

    case ALL_CONF_SCORE:
      score_function = [](const rule& x) {
        flexible_type score = all_confidence_score(x.LHS_support,
                                                   x.RHS_support,
                                                   x.total_support);
        return score;};
      break;

    case MAX_CONF_SCORE:
      score_function = [](const rule& x) {
        flexible_type score = max_confidence_score(x.LHS_support,
                                                   x.RHS_support,
                                                   x.total_support);
        return score;};
      break;

    case KULC_SCORE:
      score_function = [](const rule& x) {
        flexible_type score = kulc_score(x.LHS_support,
                                         x.RHS_support,
                                         x.total_support);
        return score;};
      break;

    case COSINE_SCORE:
      score_function = [](const rule& x) {
        flexible_type score = cosine_score(x.LHS_support,
                                           x.RHS_support,
                                           x.total_support);
        return score;};
      break;

    case CONVICTION_SCORE:
      score_function = [&num_transactions](const rule& x) {
        flexible_type score = conviction_score(x.LHS_support/((double) num_transactions),
                                               x.RHS_support/((double) num_transactions),
                                               x.total_support/((double) num_transactions));
        return score;};
      break;

    default:
      log_and_throw("Unrecognized score_type");
      break;

  }
  return score_function;

}

double confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double confidence = total_support / LHS_support;
  return confidence;
}

double lift_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double lift = total_support / (LHS_support * RHS_support);
  return lift;
}

double all_confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double all_confidence = total_support / std::max(LHS_support, RHS_support);
  return all_confidence;
}

double max_confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double max_confidence = total_support / std::min(LHS_support, RHS_support);
  return max_confidence;
}

double kulc_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double kulc = 0.5 * ((total_support / LHS_support) + (total_support / RHS_support));
  return kulc;
}

double cosine_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double cosine = total_support / std::sqrt(LHS_support * RHS_support);
  return cosine;
}

double conviction_score(const double& LHS_support, const double& RHS_support, const double& total_support){
  double conviction = (1 - RHS_support) / (1 - total_support/LHS_support);
  return conviction;
}

} // pattern_mining
} // turicreate
