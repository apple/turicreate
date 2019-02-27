/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEXT_ALIAS_H_
#define TURI_TEXT_ALIAS_H_

#include <vector>
#include <random/alias.hpp>
#include <unity/toolkits/util/spmat.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <export.hpp>
#include <unity/toolkits/text/topic_model.hpp>

/**
TODO:
- Replace spmat with flex_dict for each document
- Parallelize over documents. Trim zeros at the end of each sample.
- Try map, hopscotch_map for spmat.
- change predict_counts and sample_counts API to be able to handle
  both training set and validation set? That way we aren't starting from
  scratch on the validation set.
- combine all the word alias computation into one method
- use Eigen Vector instead of matrices with one row.
- Make sure to use const auto& where appropriate.
- Choose whether to use w, s, t, d, psdw, etc.
- Track MH acceptance ratio
- Consider using row-order Eigen matrices and checking for speedup
  (at least for CGS word_topic_counts?)
 */

namespace turi {

namespace text {


/**
 *
 *  The basic pseudocode for the AliasDLA method is as follows:
 *
initialize n_{t,w}
for w in vocab:
   compute q_w(t) for all t
   compute Q_w = sum_t q_w(t)
   A = GenerateAlias(q_w, K)
   for k = 1:K
        S_w.push(SampleAlias(A, K))
   store q_w(t), Q_w, S_w

for d in docs:
    for i in len(d):
        w = i'th word in d
        s = current topic for w in doc d
        decrement n_{s,d} and n_{s,w} by 1
        for z where n_{z,d} != 0
            compute p_dw(z)
            compute P_dw
        t = sample from q(t) by popping from S_w
        if S_w empty:
            Recompute A and populate S_w
            Recompute q_w(t), Q_w
        compute pi
        if not rand(1) < min(1, pi)
            t = s
        increment n_{t,d} and n_{t,w} by 1
*/


class EXPORT alias_topic_model : public topic_model {

 public:

  static constexpr size_t ALIAS_TOPIC_MODEL_VERSION = 1;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~alias_topic_model();

  /**
   * Clone objects to a topic_model class
   */
  topic_model* topic_model_clone() override;

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override;

  inline size_t get_version() const override {
    return ALIAS_TOPIC_MODEL_VERSION;
  }

  /**
  * Turi serialization save
  */
  void save_impl(turi::oarchive& oarc) const override;

  /**
  * Turi serialization save
  */
  void load_version(turi::iarchive& iarc, size_t version) override;


  /**
   * Train the model using the method described in (Li, 2014).
   *
   */
  void train(std::shared_ptr<sarray<flexible_type> > data, bool verbose) override;

  /**
   * Use the dataset to create an initial set of topic assignments.
   * Each element is a vector whose length is the total number of
   * words in the respective document. If the first word occurs
   * M times, then the first M elements of this vector are the
   * latent assignments for that word.
   * While sampling new assignments, topic_counts and
   * doc_topic_counts are incremented.
   */
  std::shared_ptr<sarray<std::vector<size_t>>>
    forward_sample(v2::ml_data d);

  /**
   * For the given word do the following:
   * - Compute q_w(t) and Q_w for word w. Stores this in members q and Q.
   * - Compute the alias datastructures for each word w.
   * - Fill the cache of topic samples, S_w.
   */
  void cache_word_pmf_and_samples(size_t w);

  /**
   * Simultaneously iterate through an v2::ml_data object and the sarray of
   * latent topic assignments. For each instance of a word, resample its topic.
   */
  std::map<std::string, size_t>  sample_counts(v2::ml_data d, size_t num_blocks);

  /**
   * Perform sampling given a block of data d (typically a slice
   * of an SArray represnted via an ml_data object).
   */
  void sample_block(const v2::ml_data& d,
                  std::vector<std::vector<size_t>>& doc_assignments);

  /**
   * Sample a new topic for word w in document d.
   * \param document d
   * \param word w
   * \param initial topic s
   * \param vector of topic probabilities that gets used for sampling
   *
   */
  size_t sample_topic(size_t d, size_t w, size_t s,
                      std::vector<double>& pd);

 private:
  std::shared_ptr<sarray<std::vector<size_t>>> assignments;

  spmat doc_topic_counts;
  count_vector_type topic_counts;

  // Initialize counter for tracking sampling statistics.
  atomic<size_t> token_count;

  // Word pdf datastructures
  Eigen::MatrixXd q;  // pmf for each word
  Eigen::MatrixXd Q;  // normalizing const for each word
  std::vector<random::alias_sampler> word_samplers;
  std::vector<std::vector<size_t>> word_samples;

  // Constants
  size_t TARGET_BLOCK_NUM_ELEMENTS = 1000000000/16; // approx 1gb in memory per block

 public: 

  // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("alias_topic_model")
  REGISTER_CLASS_MEMBER_FUNCTION(alias_topic_model::list_fields)
  END_CLASS_MEMBER_REGISTRATION

}; 
}
}
#endif
