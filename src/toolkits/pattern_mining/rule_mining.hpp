/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_RULE_MINING_H
#define TURI_FP_RULE_MINING_H

#include <vector>
#include <stack>
#include <map>
#include <string>
#include <memory>
#include <algorithm>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

#include <toolkits/pattern_mining/fp_node.hpp>
#include <toolkits/pattern_mining/fp_results_tree.hpp>

namespace turi{
namespace pattern_mining {

const size_t CONF_SCORE = 0;
const size_t LIFT_SCORE = 1;
const size_t ALL_CONF_SCORE = 2;
const size_t MAX_CONF_SCORE = 3;
const size_t KULC_SCORE = 4;
const size_t COSINE_SCORE = 5;
const size_t CONVICTION_SCORE = 6;

// Helper Data Structures
/**
 * rule structure
 * Collection of information used for association rule mining and prediction.
 *
 * A rule is (LHS -> RHS). Both LHS and RHS are frequent itemsets.
 *
 * Vars:
 *   LHS (vector of size_t) - item ids in the current itemset
 *   RHS (vector of size_t) - item ids to add to the current itemset
 *   LHS_support (size_t) - support of LHS
 *   RHS_support (size_t) - support of RHS
 *   total_support (size_t) - support of LHS + RHS
 *   num_transactions (size_t) - normalizing constant for support values
 */
struct rule {
  std::vector<size_t> LHS;
  std::vector<size_t> RHS;
  size_t LHS_support;
  size_t RHS_support;
  size_t total_support;
  size_t num_transactions;
};

/**
 * rule_list class
 * A collection of rule structs
 */
class rule_list {
  public:
    std::vector<rule> rules;
    size_t num_transactions;

    void add_rule(const rule& new_rule) {rules.push_back(new_rule);}
    std::vector<size_t> get_LHS_supports() const;
    std::vector<size_t> get_RHS_supports() const;
    std::vector<size_t> get_total_supports() const;
    inline size_t size() const { return rules.size(); };


    void append_rule_list(const rule_list& other_list);

    // Convert to gl_sframe
    gl_sframe to_gl_sframe(
        const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

    // Convert to flex_list
    flex_list to_flex_list(std::vector<double> scores, \
        const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

    /**
     * Extract top_k rules
     *
     * Args:
     *   top_k (size_t) - maximum number of rules to return
     *   score_type (size_t) - see score_rules() for explanation of score types
     *   indexer (topk_indexer) - convert itemset ids to items
     * Returns:
     *   top_rules (vector of flex_list) - the top k rules as
     *       flex_list{ [LHS] , [RHS], score}
     */
     flex_list get_top_k_rules(const size_t& top_k = TOP_K_MAX, \
          const size_t& score_type = CONF_SCORE,\
          const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

    std::vector<double> score_rules(const size_t& score_type) const;

};
/**
 * Provides printing of the rule_list
 */
std::ostream& operator<<(std::ostream& out, const rule_list& my_rules);


/**
 * Extract all rules
 *
 * Args:
 *   closed_itemset_tree (fp_results_tree) - compressed closed itemsets
 *
 * Returns:
 *   all_rules (rule_list) where the LHS are subsets of my_itemset
 *
 */
//rule_list extract_all_rules(const fp_results_tree& closed_itemset_tree);


/**
 * Extract the rules for a given itemset
 *
 * Args:
 *   my_itemset (vector of size_t) - itemset to mine
 *   closed_itemset_tree (fp_results_tree) - compressed closed itemsets
 *
 * Returns:
 *   my_rules (rule_list) where the LHS are subsets of my_itemset
 */
rule_list extract_relevant_rules(const std::vector<size_t>& my_itemset, \
    const fp_results_tree& closed_itemset_tree);

/**
 * Extract the top k rules for a given itemset
 *
 * Args:
 *   my_itemset (vector of size_t) - itemset to mine
 *   closed_itemset_tree (fp_results_tree) - compressed closed itemsets
 *   top_k (size_t) - maximum number of rules to mine
 *   score_type (size_t) - score type (see score_type())
 *
 * Returns:
 *   top_rules (vector of flex_list) - the top k rules as
 *       flex_list{ [LHS] , [RHS], score}
 */
flex_list extract_top_k_rules(const std::vector<size_t>& my_itemset, \
   const fp_results_tree& closed_itemset_tree, \
   const size_t& top_k = TOP_K_MAX, \
   const size_t& score_type = CONF_SCORE, \
   const std::shared_ptr<topk_indexer>& indexer = nullptr);



// Helper class for extracting rules // TODO REFACTOR
class rule_miner {
  private:
    std::vector<size_t> RHS;
    std::vector<std::vector<size_t>> LHS_list;
    std::vector<size_t> LHS_support_list;
    std::vector<std::vector<size_t>> itemset_list;
    rule_list my_rules;
  public:
    fp_results_tree closed_itemset_tree;

    rule_miner(const std::vector<size_t>& sorted_itemset,\
        const fp_results_tree& my_results);

    void extract_relevant_rules_helper(std::shared_ptr<fp_node>& node);

    rule_list get_rule_list() {return my_rules;}
    std::vector<size_t> get_itemset() {return itemset_list.front();}
};


// Helper functions for extract_top_k_rules
/**
 * Compare rule_score_pairs using the score
 */
struct rule_score_compare {
  inline bool operator()(const std::pair<rule, double> & left, \
      const std::pair<rule, double> &right){
    return left.second > right.second;
  };
};
/**
 * Rule-Score-Pair min_heap implementation
 */
class rule_score_min_heap {
  public:
    size_t top_k;
    std::priority_queue<std::pair<rule, double>, \
                        std::vector<std::pair<rule, double>>, \
                        rule_score_compare>  \
                        min_heap;

    inline rule_score_min_heap(const size_t& k = TOP_K_MAX){ top_k = k;};

    void add_rule_score_pair(const std::pair<rule, double>& rule_score_pair);
    std::vector<std::pair<rule, double>> convert_to_sorted_vector();
};
flex_list rules_to_flex_list(std::vector<std::pair<rule, double>> rule_score_pairs,
    const std::shared_ptr<topk_indexer>& indexer);


// Score functions for Association Rules ////////////////////////////////////
// See "Data Mining - Concepts and Techniques 3rd Edition Chapter 6" by
//   Jiawei Han, Micheline Kamber, and Jian Pei
/**
 * Function for scoring an sframe of rules
 * Args:
 *   sf_rules (gl_sframe) - sframe of rules
 *   score_type (size_t) - use one of the score macros below
 * Returns:
 *   sa_scores (gl_sarray) - sarray of scores for each rule
 *
 *   CONF_SCORE
 *     confidence = Support(LHS + RHS) / Support(LHS) = "Pr(RHS | LHS)"
 *
 *   LIFT_SCORE
 *     lift = Support(LHS + RHS) / (Support(LHS) * Support(RHS))
 *          = "Pr(RHS | LHS) / Pr(RHS)"
 *
 *   ALL_CONF_SCORE
 *     all_confidence = Support(LHS + RHS) / max{ Support(LHS), Support(RHS) }
 *                    = min { "Pr(RHS | LHS)" , "Pr(LHS | RHS)" }
 *
 *   MAX_CONF_SCORE
 *     max_confidence = Support(LHS + RHS) / min{ Support(LHS), Support(RHS) }
 *                    = max { "Pr(RHS | LHS)" , "Pr(LHS | RHS)" }
 *
 *   KULC_SCORE
 *     Kulczynski = 0.5 * ( "Pr(RHS | LHS)" + "Pr(LHS | RHS)" )
 *
 *   COSINE_SCORE
 *     cosine = Support(LHS + RHS) / sqrt( Support(LHS) * Support(RHS) )
 *            = sqrt( "Pr(RHS | LHS)" * "Pr(LHS | RHS)" )
 *
 *   CONVICTION_SCORE
 *     conviction = (1 - Support(RHS))/(1 - Conf(LHS => RHS))
 */

/**
 * Return the type (in the enum) for the score function from its name.
 */
inline size_t get_score_function_type_from_name(const std::string& score_function_name){
  if (score_function_name == "confidence") {
    return CONF_SCORE;
  } else if (score_function_name == "lift") {
    return LIFT_SCORE;
  } else if (score_function_name == "all_confidence") {
    return ALL_CONF_SCORE;
  } else if (score_function_name == "max_confidence") {
    return MAX_CONF_SCORE;
  } else if (score_function_name == "kulczynski") {
    return KULC_SCORE;
  } else if (score_function_name == "cosine") {
    return COSINE_SCORE;
  } else if (score_function_name == "conviction"){
    return CONVICTION_SCORE;
  } else {
    log_and_throw("Internal error. No such scoring function exists.");
  }
}


// Helper function for score_rules
std::function<double (const rule&)> get_score_function(const size_t& score_type, \
    const size_t& num_transactions);
double confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double lift_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double all_confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double max_confidence_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double kulc_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double cosine_score(const double& LHS_support, const double& RHS_support, const double& total_support);
double conviction_score(const double& LHS_support, const double& RHS_support, const double& total_support);

} // pattern_mining
} // turicreate

#endif
