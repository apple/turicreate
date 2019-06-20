/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_RANKING_SGD_SOLVER_BASE_CLASS_H_
#define TURI_SGD_RANKING_SGD_SOLVER_BASE_CLASS_H_

#include <map>
#include <vector>
#include <type_traits>
#include <core/util/code_optimization.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/sgd/sgd_solver_base.hpp>
#include <toolkits/factorization/loss_model_profiles.hpp>

namespace turi { namespace factorization {


/** The main parts of the ranking sgd solver class.  Part of the
 *  functionality is implemented in one of two subclasses,
 *  explicit_ranking_sgd_solver and implicit_ranking_sgd_solver.
 *
 *  The ranking SGD stuff is broken into two categories; ranking
 *  regularization alongside training to predict a target, and "pure"
 *  ranking in which there is no target.  Both require a substantial
 *  amount of bookkeeping to handle the negative item examples.  The
 *  details of how these are applied, however, is handled with the
 *  individual subclasses.
 */
template <class SGDInterface>
class ranking_sgd_solver_base : public sgd::sgd_solver_base {
 protected:

  const size_t max_n_threads;
  double num_sampled_negative_examples;
  size_t random_seed = 0;

 protected:

  /**
   * Constructor
   */
  ranking_sgd_solver_base(
      const std::shared_ptr<sgd::sgd_interface_base>& main_interface,
      const v2::ml_data& train_data,
      const std::map<std::string, flexible_type>& options)

      : sgd::sgd_solver_base(main_interface, train_data, options)
      , max_n_threads(thread::cpu_count())
      , num_sampled_negative_examples(options.at("num_sampled_negative_examples"))
      , random_seed(hash64(options.at("random_seed")))
  {
    DASSERT_GE(num_sampled_negative_examples, 1);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Typedefs for the x buffer.
  //
  // A buffer contains all the items for a given user. This
  // information is needed for constructing the buffer of negative
  // examples, as items not in this buffer must be picked for this
  // process.

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Virtual functions needed to be subclassed.

  /** The main method needed to be implemented by the subclass to
   *  run the ranking sgd stuff.
   *
   *  \param[in] thread_idx The thread index determining this block.
   *
   *  \param[in] num_threads The number of threads.
   *
   *  \param[in] data The v2::ml_data instance we're working with.
   *  Primarily needed for the metadata.
   *
   *  \param[in] it_init The iterator inializer for the
   *  ml_data_block_iterator used for this thread.
   *
   *  \param[in] iface The working SGD interface.
   *
   *  \param[in] step_size The current SGD step size, set by the
   *  higher level algorithm.
   *
   *  \param[in,out] error_detected If set to true, a numerical error
   *  is detected.
   *
   *  \return (loss, rank_loss) -- loss is the cumulative estimated
   *  loss value for this thread on predicting the training data, and
   *  rank_loss is the weighted loss on the negative examples.
   */
  virtual std::pair<double, double> run_sgd_thread(
      size_t iteration,
      size_t thread_idx, size_t num_threads,
      size_t block_idx, size_t num_blocks,
      const v2::ml_data& data,
      SGDInterface* iface,
      double step_size,
      volatile bool& error_detected) = 0;



  /** Calculate the loss value for the block of data assigned to a
   *  particular thread.
   *
   *  \param[in] thread_idx The thread index determining this block.
   *
   *  \param[in] num_threads The number of threads.
   *
   *  \param[in] data The ml_data instance we're working with.
   *  Primarily needed for the metadata.
   *
   *  \param[in] it_init The iterator inializer for the
   *  ml_data_block_iterator used for this thread.
   *
   *  \param[in] iface The working SGD interface.
   *
   *  \return (loss, rank_loss) -- loss is the cumulative estimated
   *  loss value for this thread on predicting the training data, and
   *  rank_loss is the weighted loss on the negative examples.
   */
  virtual std::pair<double, double> run_loss_calculation_thread(
      size_t thread_idx, size_t num_threads,
      const v2::ml_data& data,
      SGDInterface* iface) const = 0;


 public:

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  The primary functions that wrap the above virtual functions.

  /**
   *  Run a single SGD pass through the data.  Implementation of base
   *  sgd_solver's required virtual function.
   *
   *  \param[in] iteration The iteration index; what gets reported in
   *  the progress message.
   *
   *  \param[in] iface_base The interface class that gives the
   *  gradient calculation routines on top of the model definition.
   *  This must be upcast to SGDInterface*.
   *
   *  \param[in] row_start The starting row in the training data to use.  In
   *  trial mode, we are likely looking at only a subset of the data.
   *
   *  \param[in] row_end The ending row in the training data to use.  In trial
   *  mode, we are likely looking at only a subset of the data.
   *
   *  \param[in] trial_mode If true, immediately return failure on any
   *  numerical issues and do not report progress messages.
   */
  std::pair<double, double> run_iteration(
      size_t iteration,
      sgd::sgd_interface_base* model_iface,
      const v2::ml_data& data,
      double step_size) {

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1. Set up a few preliminary variables

    SGDInterface* iface = dynamic_cast<SGDInterface*>(model_iface);

    const size_t data_size = data.size();

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2. Do one parallel pass through the data

    // We accumulate the loss values from each thread here, calculated
    // from the state before the update is applied.  The total loss
    // value is used to report back an estimate of the current state.
    //
    // The objective value for the threads without
    std::vector<double> loss_values(max_n_threads, 0);

    std::vector<double> rank_loss_values(max_n_threads, 0);

    // The status is governed by a common variable.  If any of the
    // threads detects a numerical error, this flag is set and all the
    // threads soon exit.
    volatile bool error_detected = false;

    iface->setup_iteration(iteration, step_size);

    // Slice up the initial input data, so we take it from different
    // sections each time.  Since ml_data has a block cache manager
    // and other
    size_t num_blocks = 16*thread::cpu_count();

    std::vector<size_t> blocks_to_use(num_blocks);
    std::iota(blocks_to_use.begin(), blocks_to_use.end(), 0);
    random::shuffle(blocks_to_use);

    atomic<size_t> current_block = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads) {
        random::seed(hash64(thread_idx, iteration, random_seed));

        while(!error_detected) {
          size_t block_lookup_idx = (++current_block) - 1;

          // we're done in this case.
          if(block_lookup_idx >= num_blocks)
            break;

          double lv, rlv;

          std::tie(lv, rlv)
              = run_sgd_thread(
                  iteration,
                  thread_idx, num_threads,
                  blocks_to_use[block_lookup_idx], num_blocks,
                  data, iface, step_size, error_detected);

          loss_values[thread_idx] += lv;
          rank_loss_values[thread_idx] += rlv;
        }
      });

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3. Check for errors.

    if(error_detected)
      return {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};

    ////////////////////////////////////////////////////////////////////////////////
    // Step 4.  Calculate how well we've done and what the current
    // estimated value is.

    iface->finalize_iteration();

    double loss_no_regularization
        = (std::accumulate(loss_values.begin(), loss_values.end(), double(0.0))
           / std::max(size_t(1), data_size));

    double rank_loss
        = (std::accumulate(rank_loss_values.begin(), rank_loss_values.end(), double(0.0))
           / std::max(size_t(1), data_size));

    double regularization_penalty = iface->current_regularization_penalty();
    double objective_value_estimate = loss_no_regularization + rank_loss + regularization_penalty;

    // Is it a trivial model?  If so, we can break early.
    if(objective_value_estimate <= 1e-16) {
      return {0, 0};
    }

    double reported_training_loss = iface->loss_model.reported_loss_value(loss_no_regularization);

    ////////////////////////////////////////////////////////////////////////////////
    // Step 5.  Unless we're in trial mode, report progress.

    return {objective_value_estimate, reported_training_loss};
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Calculate the objective value of the current state.
   *
   *  \param[in] iface_base The interface class that gives the
   *  gradient calculation routines on top of the model definition.
   *  This must be upcast to SGDInterface*.
   *
   *  \param[in] row_start The starting row in the training data to
   *  use.  In trial mode, we are likely looking at only a subset of
   *  the data.
   *
   *  \param[in] row_end The ending row in the training data to use.
   *  In trial mode, we are likely looking at only a subset of the
   *  data.
   */
  std::pair<double, double> calculate_objective(
      sgd::sgd_interface_base* model_iface,
      const v2::ml_data& data,
      size_t iteration) const GL_HOT {

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1. Set up a few preliminary variables

    SGDInterface* iface = dynamic_cast<SGDInterface*>(model_iface);

    const size_t data_size = data.size();

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2. Do one parallel pass through the data, calculating the
    // loss value for each data point.

    std::vector<double> loss_values(max_n_threads, 0);
    std::vector<double> rank_loss_values(max_n_threads, 0);

    volatile bool error_detected = false;

    in_parallel([&](size_t thread_idx, size_t num_threads) {
        random::seed(hash64(thread_idx, iteration, random_seed));

        double loss = 0, rank_loss = 0;

        // If the training data has a target
        std::tie(loss, rank_loss) = run_loss_calculation_thread(
            thread_idx, num_threads, data, iface);

        if(!std::isfinite(loss) || loss == std::numeric_limits<double>::max()) {
          error_detected = true;
        } else {
          loss_values[thread_idx] = loss;
          rank_loss_values[thread_idx] = rank_loss;
        }
      });

    if(error_detected)
      return {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3.  Calculate the regularization penalty and the rest of
    // the objective stuff.

    double loss_no_regularization
        = (std::accumulate(loss_values.begin(), loss_values.end(), double(0.0))
           / std::max(size_t(1), data_size));

    double rank_loss
        = (std::accumulate(rank_loss_values.begin(), rank_loss_values.end(), double(0.0))
           / std::max(size_t(1), data_size));

    double regularization_penalty = iface->current_regularization_penalty();
    double objective_value_estimate = loss_no_regularization + rank_loss + regularization_penalty;

    double reported_training_loss = iface->loss_model.reported_loss_value(loss_no_regularization);

    return {objective_value_estimate, reported_training_loss};
  }

 protected:


  ////////////////////////////////////////////////////////////////////////////////

  /** Fill a buffer with (observation, target value) pairs.  Because
   * of the user-block nature of the ml_data_block_iterator, this
   * buffer is gauranteed to hold all the items rated by a particular
   * user.  If no target_value is present, then "1" is used.
   *
   * \param[out] x_buffer The buffer where the (observation,
   * target_value) pairs are stored.
   *
   * \param[in,out] it The current block iterator.
   *
   * \param[in,out] item_observed A mask giving the items observed in
   * the data.
   *
   * \return (n_rows, n_rated_items).  The number of rows in the
   * buffer, and the number of unique rated items.
   */
  inline std::pair<size_t, size_t>
  fill_x_buffer_with_users_items(
      std::vector<std::pair<std::vector<v2::ml_data_entry>, double> >& x_buffer,
      v2::ml_data_block_iterator& it,
      size_t n_items,
      dense_bitset& item_observed) const GL_HOT_INLINE_FLATTEN {

    size_t n_rows = 0;
    size_t n_rated_items = 0;

    do {
      size_t index = n_rows;

      if(x_buffer.size() <= index)
        x_buffer.resize(2*index);

      auto& x = x_buffer[index].first;
      it.fill_observation(x);

      x_buffer[index].second = it.target_value();

      size_t item = x[1].index;
      DASSERT_LT(item, n_items);

      bool old_bit = item_observed.set_bit_unsync(item);
      if(!old_bit)
        ++n_rated_items;

      ++n_rows, ++it;
    } while(!it.done() && !it.is_start_of_new_block());

    return std::make_pair(n_rows, n_rated_items);
  }


  /********************************************************************************

   ============================================================
   Internal bookkeeping routines.
   ============================================================

   These routines take care of the bookkeeping surrounding the
   tracking of items that the users have not rated.

  ********************************************************************************/

  /**  A processing buffer for the choose_negative_example function so
   *  that we can avoid memory allocations.
   */
  struct neg_sample_proc_buffer {
    std::vector<v2::ml_data_entry> candidate_x;
    std::vector<size_t> chosen_negative_items;
    std::vector<size_t> candidate_negative_items;

    // If a user has rated most of the items, then rejection sampling
    // doesn't work efficiently.  In this case, build a list of the
    // available items, then simply sample the available items from
    // that.
    //
    // This operation only has to be done once per user; thus we keep
    // the result in this thread-local buffer.  On future attempts, if
    // the user_of_available_item_list matches the current user, then
    // we skip the rejection sampling altogether and just draw from
    // this item list.
    size_t user_of_available_item_list;
    std::vector<size_t> available_item_list;
    std::vector<size_t> available_item_list_chosen_indices;
  };

  ////////////////////////////////////////////////////////////////////////////////

  /* Chooses a negative example to complement the current example,
   * given a mask of all the observed items.  Returns true if
   * successful and false if no viable negative candidate is found.
   */
  inline double choose_negative_example(
      size_t thread_idx,
      const v2::ml_data& data,
      SGDInterface* iface,
      std::vector<v2::ml_data_entry>& negative_example_x,
      const std::vector<v2::ml_data_entry>& current_positive_example,
      const dense_bitset& item_observed,
      size_t n_rows, size_t n_items,
      size_t n_rated_items,
      neg_sample_proc_buffer& proc_buf) const GL_HOT_INLINE_FLATTEN {

    const size_t ITEM_COLUMN_INDEX = 1;

    ////////////////////////////////////////////////////////////////////////////////
    // Goal: pick at most n_points with items not in the candidate
    // training set.

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1: set up the buffers

    std::vector<size_t>& chosen_negative_items = proc_buf.chosen_negative_items;
    chosen_negative_items.resize(num_sampled_negative_examples);

    std::vector<size_t>& candidate_negative_items = proc_buf.candidate_negative_items;
    candidate_negative_items.resize(num_sampled_negative_examples);

    size_t n_points_picked = 0;
    bool remove_from_available_item_list = false;

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1: Pick num_sampled_negative_examples from samples that
    // are not ones the user rated.  For efficiency, we need to handle
    // two cases -- when the user has rated few items and when the
    // user has rated many of the items.

    ////////////////////////////////////////
    //  Case 1: Fewer rated items
    //
    // If at least 1 / 8 of the items are free, then don't bother to
    // build the list of available items and just do rejection
    // sampling.  Otherwise, we should build the list of available
    // items and sample from that.

    if(8 * (n_items - n_rated_items) > n_items) {
      while(n_points_picked < num_sampled_negative_examples) {
        // Get num_sampled_negative_examples candidate points.

        for(size_t i = 0; i < num_sampled_negative_examples; ++i) {
          size_t candidate_item = random::fast_uniform<size_t>(0, n_items - 1);
          item_observed.prefetch(candidate_item);
          candidate_negative_items[i] = candidate_item;
        }

        // Move unobserved items over to the chosen points.
        for(size_t i = 0;
            i < num_sampled_negative_examples && n_points_picked < num_sampled_negative_examples;
            ++i) {

          size_t candidate_item = candidate_negative_items[i];
          if(!item_observed.get(candidate_item))
            chosen_negative_items[n_points_picked++] = candidate_item;
        }
      }

    } else {

      ////////////////////////////////////////
      // Case 2: Many rated items
      //
      // If the user has rated at least 7 / 8 of the items, then build a
      // list of the free items and randomly sample from those.  This
      // list is saved in the buffer for all subsequent rounds of
      // choosing negative example items for this user.

      size_t user = current_positive_example[0].index;

      ////////////////////////////////////////
      // Step 2.1: Build the list of items, if necessary.

      if(proc_buf.user_of_available_item_list != user
         || proc_buf.available_item_list.empty() ) {

        // The list has not been built yet; need to construct it.
        DASSERT_LT(n_rated_items, n_items);

        proc_buf.available_item_list.resize(n_items - n_rated_items);

        size_t current_position = 0;

        // Do one round of first_zero_bit to get the location of the
        // first zero bit.  Then use next_zero_bit to walk the rest.
#ifndef NDEBUG
        bool has_free_bit =
#endif
            item_observed.first_zero_bit(current_position);

        DASSERT_TRUE(has_free_bit);
        proc_buf.available_item_list[0] = current_position;

        size_t index_count = 1;

        while(true) {
          bool found_index = item_observed.next_zero_bit(current_position);

          if(found_index) {
            proc_buf.available_item_list[index_count++] = current_position;
          } else {
            break;
          }
        }

        DASSERT_EQ(index_count, n_items - n_rated_items);

        // This flags it as available for use on the next sampling
        proc_buf.user_of_available_item_list = user;
        proc_buf.available_item_list.resize(index_count);
      }

      ////////////////////////////////////////
      // Step 2.2: Sample randomly from the free items.

      proc_buf.available_item_list_chosen_indices.resize(num_sampled_negative_examples);
      for(size_t i = 0; i < num_sampled_negative_examples; ++i) {
        size_t idx = random::fast_uniform<size_t>(0, proc_buf.available_item_list.size()-1);
        chosen_negative_items[i] = proc_buf.available_item_list[idx];
        proc_buf.available_item_list_chosen_indices[i] = idx;
        DASSERT_FALSE(item_observed.get(chosen_negative_items[i]));
      }

      remove_from_available_item_list = true;
      n_points_picked = num_sampled_negative_examples;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3. Check that all the examples chosen are negative ones.

#ifndef NDEBUG
    {
      DASSERT_EQ(n_points_picked, num_sampled_negative_examples);

      for(size_t i = 0; i < n_points_picked; ++i) {
        DASSERT_FALSE(item_observed.get(chosen_negative_items[i]));
      }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////
    // Step 4: Score all the items; keep track of the highest scoring
    // one.

    std::vector<v2::ml_data_entry>& candidate_x = proc_buf.candidate_x;
    candidate_x = current_positive_example;
    size_t trim_size = candidate_x.size();

    if(data.has_side_features()) {
      // Strip out the side features associated with the item row;
      // they will get added in later.

      size_t lb, ub;
      std::tie(lb, ub) =
          data.get_side_features()->column_indices_of_side_information_block(ITEM_COLUMN_INDEX);

      auto new_end = std::remove_if(candidate_x.begin(), candidate_x.end(),
                                    [&](const v2::ml_data_entry& v) {
                                      return (lb <= v.column_index) && (v.column_index < ub);
                                    });

      trim_size = new_end - candidate_x.begin();
    }

    double highest_fx = std::numeric_limits<double>::lowest();
    size_t chosen_idx = 0;

    // Build the item segments.
    for(size_t i = 0; i < n_points_picked; ++i) {
      candidate_x[1].index = chosen_negative_items[i];

      // Add in the side information as needed
      if(data.has_side_features()) {
        candidate_x.resize(trim_size);
        data.get_side_features()->add_partial_side_features_to_row(candidate_x, ITEM_COLUMN_INDEX);
      }

      double fx_hat = iface->calculate_fx(thread_idx, candidate_x);

      // We hit a numerical error. Baaaaad.
      if(!std::isfinite(fx_hat))
        return NAN;

      if(fx_hat > highest_fx) {
        highest_fx = fx_hat;
        negative_example_x = candidate_x;
        chosen_idx = i;
      }
    }

    // We hit a numerical error. Baaaaad.
    if(highest_fx == std::numeric_limits<double>::lowest())
      return NAN;

    if(remove_from_available_item_list) {
      DASSERT_LT(chosen_idx, proc_buf.available_item_list_chosen_indices.size());
      size_t remove_idx = proc_buf.available_item_list_chosen_indices[chosen_idx];
      DASSERT_LT(remove_idx, proc_buf.available_item_list.size());
      std::swap(proc_buf.available_item_list[remove_idx], proc_buf.available_item_list.back());
      proc_buf.available_item_list.pop_back();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 5: Return the value.  Means we are done!

    return highest_fx;
  }

  /** Clear out the item_observed buffer.
   *
   *  Based on the number of items actually used, deletes stuff.
   *  Defined below.
   *
   */
  template <typename BufferIndexToItemIndexMapper>
  GL_HOT_INLINE_FLATTEN
  inline void clear_item_observed_buffer(
      dense_bitset& item_observed, size_t n_rows, size_t n_items,
      const BufferIndexToItemIndexMapper& map_index) const {

    // If the number of on items means that less than 1/8 of the
    // bit-words are going to be touched, then just clear those
    // selectively. Thus we optimize it for sparse arrays. Otherwise,
    // it is faster to just memset the entire array.

    if(n_rows < n_items / ( 8*sizeof(size_t) * 8) ) {
      for(size_t i = 0; i < n_rows; ++i) {
        size_t index = map_index(i);
        item_observed.clear_word_unsync(index);
      }
    } else {
      item_observed.clear();
    }

    DASSERT_TRUE(item_observed.empty());
  }
};


}}

#endif
