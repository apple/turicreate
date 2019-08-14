#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/pattern_mining/rule_mining.hpp>
#include <toolkits/pattern_mining/fp_growth.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::pattern_mining;

/*
 * Note the conviction score tests are commented out. Those cannot be right.
 * Some of the "true values" are NaNs or Infs
 */

/**
 *  Run tests.
*/
struct fp_rule_mining_test  {

    rule_list setupRuleList(void) {
      rule_list my_rules;

      rule rule1;
      rule1.LHS = {1};
      rule1.RHS = {9};
      rule1.LHS_support = 10;
      rule1.RHS_support = 7;
      rule1.total_support = 5;

      rule rule2;
      rule2.LHS = {2};
      rule2.RHS = {9};
      rule2.LHS_support = 10;
      rule2.RHS_support = 5;
      rule2.total_support = 5;

      rule rule3;
      rule3.LHS = {3};
      rule3.RHS = {9};
      rule3.LHS_support = 5;
      rule3.RHS_support = 7;
      rule3.total_support = 5;

      rule rule4;
      rule4.LHS = {4};
      rule4.RHS = {9};
      rule4.LHS_support = 5;
      rule4.RHS_support = 4;
      rule4.total_support = 1;

      my_rules.add_rule(rule1);
      my_rules.add_rule(rule2);
      my_rules.add_rule(rule3);
      my_rules.add_rule(rule4);
      my_rules.num_transactions = 20;
      return my_rules;
    }

  public:

    void testConfidenceScore(void){
      TS_ASSERT_DELTA(confidence_score(10, 7, 5), 5.0 / 10.0, 1e-7);
      TS_ASSERT_DELTA(confidence_score(10, 5, 5), 5.0 / 10.0, 1e-7);
      TS_ASSERT_DELTA(confidence_score(5, 7, 5), 5.0 / 5.0, 1e-7);
      TS_ASSERT_DELTA(confidence_score(5, 4, 1), 1.0 / 5.0, 1e-7);
    }
    void testLiftScore(void){
      TS_ASSERT_DELTA(lift_score(10, 7, 5), 5.0 / 70.0, 1e-7);
      TS_ASSERT_DELTA(lift_score(10, 5, 5), 5.0 / 50.0, 1e-7);
      TS_ASSERT_DELTA(lift_score(5, 7, 5), 5.0 / 35.0, 1e-7);
      TS_ASSERT_DELTA(lift_score(5, 4, 1), 1.0 / 20.0, 1e-7);
    }
//     void testConvictionScore(void){
//       TS_ASSERT_DELTA(conviction_score(10, 7/20.0, 5), (1 - 7.0/20.0) / (1 - 5.0/10.0), 1e-7);
//       TS_ASSERT_DELTA(conviction_score(10, 5/20.0, 5), (1 - 5.0/20.0) / (1 - 5.0/10.0), 1e-7);
//       TS_ASSERT_DELTA(conviction_score(5, 7/20.0, 5), (1 - 7.0/20.0) / (1 - 5.0/5.0), 1e-7);
//       TS_ASSERT_DELTA(conviction_score(5, 4/20.0, 1), (1 - 4.0/20.0) / (1 - 1.0/5.0), 1e-7);
//     }
    void testAllConfidenceScore(void){
      TS_ASSERT_DELTA(all_confidence_score(10, 7, 5), 5.0 / 10.0, 1e-7);
      TS_ASSERT_DELTA(all_confidence_score(10, 5, 5), 5.0 / 10.0, 1e-7);
      TS_ASSERT_DELTA(all_confidence_score(5, 7, 5), 5.0 / 7.0, 1e-7);
      TS_ASSERT_DELTA(all_confidence_score(5, 4, 1), 1.0 / 5.0, 1e-7);
    }
    void testMaxConfidenceScore(void){
      TS_ASSERT_DELTA(max_confidence_score(10, 7, 5), 5.0 / 7.0, 1e-7);
      TS_ASSERT_DELTA(max_confidence_score(10, 5, 5), 5.0 / 5.0, 1e-7);
      TS_ASSERT_DELTA(max_confidence_score(5, 7, 5), 5.0 / 5.0, 1e-7);
      TS_ASSERT_DELTA(max_confidence_score(5, 4, 1), 1.0 / 4.0, 1e-7);
    }
    void testKulcScore(void){
      TS_ASSERT_DELTA(kulc_score(10, 7, 5), 0.5 * ((5.0 / 10.0) + (5.0/7.0)), 1e-7);
      TS_ASSERT_DELTA(kulc_score(10, 5, 5), 0.5 * ((5.0 / 10.0) + (5.0/5.0)), 1e-7);
      TS_ASSERT_DELTA(kulc_score(5, 7, 5), 0.5 * ((5.0 / 5.0) + (5.0/7.0)), 1e-7);
      TS_ASSERT_DELTA(kulc_score(5, 4, 1), 0.5* ((1.0 / 5.0)+ (1.0/4.0)), 1e-7);
    }
    void testCosineScore(void){
      TS_ASSERT_DELTA(cosine_score(10, 7, 5), 5.0 / std::sqrt(70.0), 1e-7);
      TS_ASSERT_DELTA(cosine_score(10, 5, 5), 5.0 / std::sqrt(50.0), 1e-7);
      TS_ASSERT_DELTA(cosine_score(5, 7, 5), 5.0 / std::sqrt(35.0), 1e-7);
      TS_ASSERT_DELTA(cosine_score(5, 4, 1), 1.0 / std::sqrt(20.0), 1e-7);
    }

    void testConfScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> conf_scores = my_rules.score_rules(CONF_SCORE);
      std::vector<double> expected_scores = { 5.0 / 10.0,
                                              5.0 / 10.0,
                                              5.0 / 5.0,
                                              1.0 / 5.0 };
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(conf_scores[i], expected_scores[i], 1e-7);
      }
    }
    void testLiftScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> lift_scores = my_rules.score_rules(LIFT_SCORE);
      std::vector<double> expected_scores = { 5.0 / 70.0 * 20.0,
                                              5.0 / 50.0 * 20.0,
                                              5.0 / 35.0 * 20.0,
                                              1.0 / 20.0 * 20.0};
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(lift_scores[i], expected_scores[i], 1e-7);
      }
    }
//     void testConvictionScoreRules(void){
//       rule_list my_rules = setupRuleList();
// 
//       std::vector<double> conviction_scores = my_rules.score_rules(CONVICTION_SCORE);
//       std::vector<double> expected_scores = { (1 - 7.0/20.0) / (1 - 5.0/10.0),
//                                               (1 - 5.0/20.0) / (1 - 5.0/10.0),
//                                               (1 - 7.0/20.0) / (1 - 5.0/5.0),
//                                               (1 - 4.0/20.0) / (1 - 1.0/5.0)};
//       for (size_t i = 0;i < expected_scores.size(); ++i) {
//         TS_ASSERT_DELTA(conviction_scores[i], expected_scores[i], 1e-7);
//       }
//     }
    void testAllConfScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> all_conf_scores = my_rules.score_rules(ALL_CONF_SCORE);
      std::vector<double> expected_scores = { 5.0 / 10.0,
                                              5.0 / 10.0,
                                              5.0 / 7.0,
                                              1.0 / 5.0};
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(all_conf_scores[i], expected_scores[i], 1e-7);
      }
    }
    void testMaxConfScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> max_conf_scores = my_rules.score_rules(MAX_CONF_SCORE);
      std::vector<double> expected_scores = { 5.0 / 7.0,
                                              5.0 / 5.0,
                                              5.0 / 5.0,
                                              1.0 / 4.0};
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(max_conf_scores[i], expected_scores[i], 1e-7);
      }
    }
    void testKulcScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> kulc_scores = my_rules.score_rules(KULC_SCORE);
      std::vector<double> expected_scores = { 0.5 * ((5.0 / 10.0) + (5.0/7.0)),
                                              0.5 * ((5.0 / 10.0) + (5.0/5.0)),
                                              0.5 * ((5.0 / 5.0) + (5.0/7.0)),
                                              0.5* ((1.0 / 5.0)+ (1.0/4.0))};
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(kulc_scores[i], expected_scores[i], 1e-7);
      }
    }
    void testCosineScoreRules(void){
      rule_list my_rules = setupRuleList();

      std::vector<double> cosine_scores = my_rules.score_rules(COSINE_SCORE);
      std::vector<double> expected_scores = { 5.0 / std::sqrt(70.0),
                                              5.0 / std::sqrt(50.0),
                                              5.0 / std::sqrt(35.0),
                                              1.0 / std::sqrt(20.0)};
      for (size_t i = 0;i < expected_scores.size(); ++i) {
        TS_ASSERT_DELTA(cosine_scores[i], expected_scores[i], 1e-7);
      }
    }

    // Test extract_relevant_rules
    void testExtractRelevantRules(void) {
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{2, 1, 4},
                                              flex_list{2, 3},
                                              flex_list{2, 3, 1, 4},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{3},
                                              flex_list{1},
                                              flex_list{1, 0},
                                              flex_list{}}},
                              {"support", {20, 24, 12, 20, 30, 27, 23, 13, 40}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      std::vector<size_t> my_itemset = {1};
      auto my_rules = extract_relevant_rules(my_itemset, my_results);
      // std::cout << my_rules << std::endl;
      TS_ASSERT_EQUALS(my_rules.rules.size(), 7);
      TS_ASSERT_EQUALS(my_rules.get_LHS_supports(), \
          std::vector<size_t>({40, 23, 40, 23, 40, 23, 23}));
      TS_ASSERT_EQUALS(my_rules.get_RHS_supports(), \
          std::vector<size_t>({30, 20, 24, 12, 27, 27, 13}));
      TS_ASSERT_EQUALS(my_rules.get_total_supports(), \
          std::vector<size_t>({30, 20, 24, 12, 27, 20, 13}));
      TS_ASSERT_EQUALS(my_rules.num_transactions, 40); // Support of emptyset

      my_itemset = {4};
      my_rules = extract_relevant_rules(my_itemset, my_results);
      // std::cout << my_rules << std::endl;
      TS_ASSERT_EQUALS(my_rules.rules.size(), 8);
      TS_ASSERT_EQUALS(my_rules.get_LHS_supports(), \
          std::vector<size_t>({40, 20, 40, 20, 40, 40, 40, 40}));
      TS_ASSERT_EQUALS(my_rules.get_RHS_supports(), \
          std::vector<size_t>({30, 20, 24, 12, 27, 20, 23, 13}));
      TS_ASSERT_EQUALS(my_rules.get_total_supports(), \
          std::vector<size_t>({30, 20, 24, 12, 27, 20, 23, 13}));



      my_itemset = {5, 3, 4};
      my_rules = extract_relevant_rules(my_itemset, my_results);
      // std::cout << my_rules << std::endl;
      TS_ASSERT_EQUALS(my_rules.rules.size(), 7);
      TS_ASSERT_EQUALS(my_rules.get_LHS_supports(), \
          std::vector<size_t>({40, 20, 27, 12, 27, 40, 40}));
      TS_ASSERT_EQUALS(my_rules.get_RHS_supports(), \
          std::vector<size_t>({30, 20, 30, 20, 23, 23, 13}));
      TS_ASSERT_EQUALS(my_rules.get_total_supports(), \
          std::vector<size_t>({30, 20, 24, 12, 20, 23, 13}));

    }

    // test rule_list::get_top_k_rules()
    void testGetTopKRules(void) {
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{2, 1, 4},
                                              flex_list{2, 3},
                                              flex_list{2, 3, 1, 4},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{3},
                                              flex_list{1},
                                              flex_list{1, 0},
                                              flex_list{}}},
                              {"support", {20, 24, 12, 20, 30, 27, 23, 13, 40}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      std::vector<size_t> my_itemset = {2, 0};
      auto my_rules = extract_relevant_rules(my_itemset, my_results);
      // std::cout << my_rules;

      flex_list _conf_rules = my_rules.get_top_k_rules(5, CONF_SCORE);
      std::vector<flex_list> conf_rules;
      for (const auto& cr: _conf_rules) {
        conf_rules.push_back(cr);
      }

      // std::cout << conf_rules;
      TS_ASSERT_EQUALS(conf_rules.size(), 5);
      // First rule is 0 -> 1
      TS_ASSERT_EQUALS(conf_rules[0][3], 13); // Support of 0
      TS_ASSERT_EQUALS(conf_rules[0][4], 23); // Support of 1
      TS_ASSERT_EQUALS(conf_rules[0][5], 13); // Support of 0,1
      TS_ASSERT_DELTA(conf_rules[0][2], 1.0, 1e-6);

      // Second rule is 2 -> 3
      TS_ASSERT_EQUALS(conf_rules[1][3], 30);
      TS_ASSERT_EQUALS(conf_rules[1][4], 27);
      TS_ASSERT_EQUALS(conf_rules[1][5], 24);
      TS_ASSERT_DELTA(conf_rules[1][2], 0.8, 1e-6);

      // Third rule is [] -> 3
      TS_ASSERT_EQUALS(conf_rules[2][3], 40);
      TS_ASSERT_EQUALS(conf_rules[2][4], 27);
      TS_ASSERT_EQUALS(conf_rules[2][5], 27);
      TS_ASSERT_DELTA(conf_rules[2][2], 0.675, 1e-6);

      flex_list _cosine_rules = my_rules.get_top_k_rules(500, COSINE_SCORE);
      std::vector<flex_list> cosine_rules;
      for (const auto& cr: _cosine_rules) {
        cosine_rules.push_back(cr);
      }

      // std::cout << cosine_rules;
      TS_ASSERT_EQUALS(cosine_rules.size(), 7);
      TS_ASSERT_EQUALS(cosine_rules[0][0], flex_list{2});
      TS_ASSERT_EQUALS(cosine_rules[0][1], flex_list{3});
      TS_ASSERT_DELTA(cosine_rules[0][2], 24.0 / std::sqrt(30*27), 1e-3);
      TS_ASSERT_EQUALS(cosine_rules[1][0], flex_list{});
      TS_ASSERT_EQUALS(cosine_rules[1][1], flex_list{3});
      TS_ASSERT_DELTA(cosine_rules[1][2], 27.0 / std::sqrt(27*40), 1e-3);

    }
    // Test extract_top_k_rules()
    void testExtractTopKRules(void) {
      std::vector<size_t> id_order = {2, 3, 1, 4, 0};
      gl_sframe closed_itemsets{{"itemsets", {flex_list{2, 1, 4},
                                              flex_list{2, 3},
                                              flex_list{2, 3, 1, 4},
                                              flex_list{3, 1},
                                              flex_list{2},
                                              flex_list{3},
                                              flex_list{1},
                                              flex_list{1, 0},
                                              flex_list{}}},
                              {"support", {20, 24, 12, 20, 30, 27, 23, 13, 40}}};
      fp_results_tree my_results = fp_results_tree(id_order);
      my_results.build_tree(closed_itemsets);

      std::vector<size_t> my_itemset = {2, 0};

      flex_list _conf_rules = extract_top_k_rules(my_itemset, my_results, 5, CONF_SCORE);
      std::vector<flex_list> conf_rules;
      for (const auto& cr: _conf_rules) {
        conf_rules.push_back(cr);
      }

      // std::cout << conf_rules;
      TS_ASSERT_EQUALS(conf_rules.size(), 5);
      TS_ASSERT_DELTA(conf_rules[0][2], 1.0, 1e-6);
      TS_ASSERT_DELTA(conf_rules[1][2], 0.8, 1e-6);
      TS_ASSERT_DELTA(conf_rules[2][2], 0.675, 1e-6);

      flex_list _cosine_rules = extract_top_k_rules(my_itemset, my_results, 500, COSINE_SCORE);
      std::vector<flex_list> cosine_rules;
      for (const auto& cr: _cosine_rules) {
        cosine_rules.push_back(cr);
      }

      // std::cout << cosine_rules;
      TS_ASSERT_EQUALS(cosine_rules.size(), 7);
      TS_ASSERT_EQUALS(cosine_rules[0][0], flex_list{2});
      TS_ASSERT_EQUALS(cosine_rules[0][1], flex_list{3});
      TS_ASSERT_DELTA(cosine_rules[0][2], 24.0 / std::sqrt(30*27), 1e-3);
      TS_ASSERT_EQUALS(cosine_rules[1][0], flex_list{});
      TS_ASSERT_EQUALS(cosine_rules[1][1], flex_list{3});
      TS_ASSERT_DELTA(cosine_rules[1][2], 27.0 / std::sqrt(27*40), 1e-3);
    }


};


BOOST_FIXTURE_TEST_SUITE(_fp_rule_mining_test, fp_rule_mining_test)
BOOST_AUTO_TEST_CASE(testConfidenceScore) {
  fp_rule_mining_test::testConfidenceScore();
}
BOOST_AUTO_TEST_CASE(testLiftScore) {
  fp_rule_mining_test::testLiftScore();
}
BOOST_AUTO_TEST_CASE(testAllConfidenceScore) {
  fp_rule_mining_test::testAllConfidenceScore();
}
BOOST_AUTO_TEST_CASE(testMaxConfidenceScore) {
  fp_rule_mining_test::testMaxConfidenceScore();
}
BOOST_AUTO_TEST_CASE(testKulcScore) {
  fp_rule_mining_test::testKulcScore();
}
BOOST_AUTO_TEST_CASE(testCosineScore) {
  fp_rule_mining_test::testCosineScore();
}
BOOST_AUTO_TEST_CASE(testConfScoreRules) {
  fp_rule_mining_test::testConfScoreRules();
}
BOOST_AUTO_TEST_CASE(testLiftScoreRules) {
  fp_rule_mining_test::testLiftScoreRules();
}
BOOST_AUTO_TEST_CASE(testAllConfScoreRules) {
  fp_rule_mining_test::testAllConfScoreRules();
}
BOOST_AUTO_TEST_CASE(testMaxConfScoreRules) {
  fp_rule_mining_test::testMaxConfScoreRules();
}
BOOST_AUTO_TEST_CASE(testKulcScoreRules) {
  fp_rule_mining_test::testKulcScoreRules();
}
BOOST_AUTO_TEST_CASE(testCosineScoreRules) {
  fp_rule_mining_test::testCosineScoreRules();
}
BOOST_AUTO_TEST_CASE(testExtractRelevantRules) {
  fp_rule_mining_test::testExtractRelevantRules();
}
BOOST_AUTO_TEST_CASE(testGetTopKRules) {
  fp_rule_mining_test::testGetTopKRules();
}
BOOST_AUTO_TEST_CASE(testExtractTopKRules) {
  fp_rule_mining_test::testExtractTopKRules();
}
BOOST_AUTO_TEST_SUITE_END()
