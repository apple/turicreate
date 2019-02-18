/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_MODEL_BASE_H_
#define TURI_FACTORIZATION_MODEL_BASE_H_

#include <Eigen/Core>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <unity/toolkits/factorization/loss_model_profiles.hpp>
#include <serialization/serialization_includes.hpp>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/variant.hpp>
#include <unity/lib/variant_deep_serialize.hpp>


namespace turi {

class sframe;
class oarchive;
class iarchive;
class option_manager;

namespace v2 {
class ml_data;
class ml_metadata;
class ml_data_side_features;
struct ml_data_entry;
}

namespace factorization {

////////////////////////////////////////////////////////////////////////////////

/** Factorization model class.
 *
 *  This class is the base model for all the factorization models.
 *  All interaction with these models should go through this class.
 *
 *  This class is intended to be embedded within other models as the
 *  matrix factorization interface.  For example, the recommender
 *  model holds a std::shared_ptr<factorization_model> pointer.  The
 *  matrix factorization class exposed to the user as a standalone
 *  model will also embed this class.
 *
 *  The details of the model are implemented in a subclass of
 *  factorization_model with template parameters controlling some
 *  aspects of the model's functionality.  In particular, if the model
 *  is a matrix factorization model, only the first two dimensions
 *  have latent factors, whereas a factorization model has latent
 *  factors for all dimensions.  This is in
 *  factorization_model_impl.hpp.
 *
 *  To train a model, use the static train(...) function below.
 *  Similarly, to load such a model, use the static load(...) function
 *  below.  These instantiate the correct type of subclass, then
 *  return a pointer to this class.
 *
 */
class factorization_model {
 public:

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Part 1: Model Training

  /** Create and train a factorization model with the given options.
   *  Uses the factory method pattern.
   *
   *  The training method constructs an
   *
   *  \param[in] factor_mode One of "linear_model",
   *  "matrix_factorization", or "factorization_model".  If
   *  "linear_model", then the class is essentially linear regression;
   *  if "matrix_factorization", then only the first two columns have
   *  latent factors, and if "factorization_model", then the full
   *  factorization machine model is used.
   *
   *  \param[in] train_data  The training data for the model.
   *
   *  \param[in] options The options used in the current model as well
   *  as training parameters.
   */
  static std::shared_ptr<factorization_model> factory_train(
      const std::string& factor_mode,
      const v2::ml_data& train_data,
      std::map<std::string, flexible_type> options);

  /** Returns a map of the training statistics of the model.
   */
  std::map<std::string, variant_type> get_training_stats() const;

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Part 3: Model options.

  /** Call the following function to insert the option definitions
   *  needed for the factorization_model class into an option manager.
   *
   *  The option_flags parameter is used to control what options are
   *  enabled and what the factorization_model class is expected to
   *  support.  Possible flags are as follows:
   *
   *  ranking: Include options for ranking-based optimization.  This
   *  is required for implicit rating optimization.
   *
   *  \param[in,out] options The option manager to add the options to.
   *
   *  \param[in] option_flags The functionality that the
   *  factorization_model class is expected to support.
   *
   *  This function is defined in factorization_model_options.cpp.
   */
  static void add_options(
      option_manager& options,
      const std::vector<std::string>& option_flags);

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Part 4: Interface methods to use the model.

  /** Calculate the value of the objective function as determined by
   *  the loss function, for a full data set, minus the regularization
   *  penalty.
   */
  double calculate_loss(const v2::ml_data& data) const;


  /** Make a prediction for every observation in test_data.  Returns a
   *  single-column SFrame with a prediction for every observation.
   */
  sframe predict(const v2::ml_data& test_data) const;

  /** Scores all the items in scores, updating the score.  Used by the
   *  recommender system.
   */
  virtual void score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const = 0;

  /**  Resets the state with an initial random seed and standard
   *  deviation.
   */
  virtual void reset_state(size_t random_seed, double sd) = 0;


  /** Returns a map of all the different coefficients of the model, as
   *  given by the current model.
   */
  virtual std::map<std::string, variant_type> get_coefficients() const = 0;

 protected:

  ////////////////////////////////////////////////////////////////////////////////
  // These functions need to be implemented by the child class.

  /** Calculate the linear function value at the given point wrt the
   *  current state.
   */
  virtual double calculate_fx(size_t thread_idx, const std::vector<v2::ml_data_entry>& x) const = 0;

  /** \overload
   */
  virtual double calculate_fx(const std::vector<v2::ml_data_entry>& x) const = 0;
  
 public:

  virtual void get_item_similarity_scores(
      size_t item, std::vector<std::pair<size_t, double> >& sim_scores) const = 0;

  typedef Eigen::Matrix<float, Eigen::Dynamic, 1> vector_type;

  /** Computes the cosine similarity between a particular factor
   * within a column and all the other factors within that column.
   */
  virtual void calculate_intracolumn_similarity(vector_type& dest, size_t column_index, size_t ref_index) const = 0;

  /** Set up the model with the correct index sizes, etc.
   *
   *  Here, only should be called from the training functions in
   *  factorization_model_training.  However, these are implemented
   *  outside of this class, so we have to keep this method public.
   */
  void setup(
      const std::string& loss_model_name,
      const v2::ml_data& train_data,
      const std::map<std::string, flexible_type>& opts);

 protected:
  
  virtual void internal_setup(const v2::ml_data& train_data) {}

  // Unfortunately, the sgd interface needs these right now :-P, so
  // keep them public for that. 
 public:
  
  // All the options for this model
  std::map<std::string, flexible_type> options;

  // These end up storing the original index blocks that the model was
  // trained on.  The length of these is equal to the number of
  // columns, with index_sizes storing the number of indices
  // (features) used at test time and index_offsets storing the offset
  // needed to easily make the local feature unique.  feature_index +
  // index_offsets[column] gives a unique global index, and
  // index_sizes allows us to detect new features.
  size_t n_total_dimensions = 0;

  std::vector<size_t> index_sizes;
  std::vector<size_t> index_offsets;

  // This gives the amount to shift and scale the columns by.  Only
  // numerical columns are shifted by default.
  std::vector<std::pair<double, double> > column_shift_scales;

  std::shared_ptr<v2::ml_metadata> metadata;

  double target_mean=0;
  double target_sd=1;

  size_t random_seed = 0;

  ////////////////////////////////////////////////////////////////////////////////

  std::string loss_model_name;
  std::shared_ptr<loss_model_profile> loss_model;

  std::map<std::string, variant_type> _training_stats;

  ////////////////////////////////////////////////////////////////////////////////
  // Part X: Serialization.
  //
  // Implementing the serialization is less trivial here, as we are
  // dealing with a base class that has a number of possible
  // subclasses.  The expectation is that everything outside this
  // class will not have to interact with the templated subclass of
  // this but rather use std::shared_ptr<factorization_model>.  The
  // serialization methods for std::shared_ptr<factorization_model>
  // should just work.
  //
  // The implementation of these functions is in
  // factorization_model_serialization.cpp.

 public:

  /** Return the serialization version. 
   */
  virtual size_t get_version() const = 0; 

  /** Serialization in factorization_model_impl.  These methods allow
   *  the child class to have specific parameters that need to be
   *  serialized and deserialized.
   */
  virtual void save_impl(turi::oarchive& oarc) const = 0;
  virtual void load_version(turi::iarchive& iarc, size_t version) = 0;

  /** Serialization of this base class.
   */
  void local_save_impl(turi::oarchive& oarc) const;
  void local_load_version(turi::iarchive& iarc, size_t version);
  
  /** Return all the parameters needed by factory_load to determine
   *  what model to instantiate.
   */
  virtual std::map<std::string, variant_type> get_serialization_parameters() const = 0;

  /** Instantiate and load a factorization model from a stream.
   */
  static std::shared_ptr<factorization_model> factory_load(
      size_t version,
      const std::map<std::string, variant_type>& serialization_parameters,
      turi::iarchive& iarc);
};

}}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for vector<std::shared_ptr<column_indexer>
// > and std::shared_ptr<column_indexer>

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<factorization::factorization_model>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number 
    size_t version = m->get_version();
    arc << version; 
    
    // Save the model parameters as a map 
    std::map<std::string, variant_type> serialization_parameters =
        m->get_serialization_parameters();

    variant_deep_save(to_variant(serialization_parameters), arc);

    m->local_save_impl(arc);
  }

} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<factorization::factorization_model>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    size_t version;
    arc >> version;
    
    variant_type data_v;
    variant_deep_load(data_v, arc);
  
    std::map<std::string, variant_type> data;
    data = variant_get_value<decltype(data)>(data_v);

    m = factorization::factorization_model::factory_load(version, data, arc);
    
  } else {
    m = std::shared_ptr<factorization::factorization_model>();
  }
} END_OUT_OF_PLACE_LOAD()


#endif
