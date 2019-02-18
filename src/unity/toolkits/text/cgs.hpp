/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEXT_CGS_H_
#define TURI_TEXT_CGS_H_

#include <vector>
#include <export.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/util/spmat.hpp>

namespace turi {

namespace text {

/**
 * Returns a random categorical variable in [0, ..., K-1] where
 * K is the length of the provided vector.
 * Modifies the provided vector to be normalized probabilities.
 */
size_t random_categorical(std::vector<double>& logprobs);

class EXPORT cgs_topic_model : public topic_model {

 public:

  static constexpr size_t CGS_TOPIC_MODEL_VERSION = 1;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~cgs_topic_model();

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
    return CGS_TOPIC_MODEL_VERSION;
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
   * Train the model using collapsed Gibbs sampling.
   *
   * For the seminal work on this, see Griffiths, Steyvers 2004.
   *
   * This algorithm is a Gibbs sampler where we sample the latent topic for
   * each word conditioned on all other latent assignments. This particular
   * algorithm is "collapsed" in the sense that sample from the conditional
   * distribution of a model where many of the parameters have been
   * analytically integrated out. This has been experimentally shown to
   * yield more (statistically) efficient samplers.
   *
   * A few departures from the vanilla version:
   * - Like several other implementations, we sample a single latent
   *   assignment z_ij per (document, word, count) token, rather than
   *   a latent assignment for every occurrence of every word. This
   *   is done for speed reasons, but it no longer is the proper
   *   distribution. It would be easy to add in a loop over the counts
   *   for each (document, word) pair.
   * - Initialization is done by "forward sampling", where we sample
   *   from the conditional distribution of each latent assignment using
   *   the assignments sampled previously. This allows us to naturally
   *   handle the case where a user has provided a set of topics for
   *   initialization purposes.
   *
   */
  void train(std::shared_ptr<sarray<flexible_type>> data, bool verbose) override;

  std::shared_ptr<sarray<std::vector<size_t>>>
      forward_sample(const v2::ml_data& d,
                     count_vector_type& topic_counts,
                     count_matrix_type& doc_topic_counts);

  std::map<std::string, size_t>
      sample_counts(const v2::ml_data& d,
                    count_vector_type& topic_counts,
                    count_matrix_type& doc_topic_counts,
                    std::shared_ptr<sarray<std::vector<size_t>>>& assignments);

  // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("cgs_topic_model")
  REGISTER_CLASS_MEMBER_FUNCTION(cgs_topic_model::list_fields)
  END_CLASS_MEMBER_REGISTRATION

};  // kmeans_model class

}
}
#endif
