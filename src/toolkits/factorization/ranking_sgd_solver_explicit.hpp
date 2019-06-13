/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_EXPLICIT_RANKING_SGD_SOLVER_CLASS_H_
#define TURI_SGD_EXPLICIT_RANKING_SGD_SOLVER_CLASS_H_

#include <map>
#include <vector>
#include <random>
#include <type_traits>
#include <core/util/code_optimization.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/factorization/ranking_sgd_solver_base.hpp>

namespace turi { namespace factorization {

/** Ranking When Target Is Present
 *
 *  ============================================================
 *
 * When the target is present, simultaneously attempt to fit the model
 * to the targets, while penalizing items that are predicted above
 * value_of_unobserved_items.
 */
template <class SGDInterface>
class explicit_ranking_sgd_solver final : public ranking_sgd_solver_base<SGDInterface> {
public:

  /** Constructor
   */
  explicit_ranking_sgd_solver(
      const std::shared_ptr<sgd::sgd_interface_base>& main_interface,
      const v2::ml_data& train_data,
      const std::map<std::string, flexible_type>& options)

      : ranking_sgd_solver_base<SGDInterface>(main_interface, train_data, options)
      , ranking_regularization(options.at("ranking_regularization"))
      , _unobserved_rating_value(options.at("unobserved_rating_value"))
      , num_sampled_negative_examples(options.at("num_sampled_negative_examples"))
      , observation_block_size(options.at("sgd_sampling_block_size"))
  {
    DASSERT_GT(ranking_regularization, 0);

    if(!(_unobserved_rating_value > std::numeric_limits<float>::lowest())
       && train_data.has_target()) {

      const auto& target_stats = train_data.metadata()->target_statistics();

      if(target_stats->stdev(0) == 0) {
        _unobserved_rating_value = target_stats->mean(0) - 1;

      } else {
        _unobserved_rating_value = (target_stats->mean(0)
                                    - 1.96 * target_stats->stdev(0));
      }
    }

    DASSERT_TRUE(std::isfinite(_unobserved_rating_value));

    // Set up all the thread-local buffers.
    size_t max_n_threads = thread::cpu_count();
    thread_x_buffers.resize(max_n_threads);
    thread_candidate_pool.resize(max_n_threads);
    thread_order_index_buffers.resize(max_n_threads);
    thread_negative_example_flags.resize(max_n_threads);
    thread_item_observed.resize(max_n_threads);
  }

private:

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Typedefs -- need to pull these here.

  typedef ranking_sgd_solver_base<SGDInterface> Base;
  typedef typename Base::neg_sample_proc_buffer neg_sample_proc_buffer;

  double ranking_regularization = 0;
  double _unobserved_rating_value = NAN;
  size_t num_sampled_negative_examples = 0;
  size_t observation_block_size = 0;

  static constexpr bool using_logistic_loss =
      std::is_same<typename SGDInterface::LossModelProfile, loss_logistic>::value;

  inline double unobserved_rating_value() const {
    return using_logistic_loss ? 0 : _unobserved_rating_value;
  }

  /// vector of (row data, target_value) pairs.
  typedef std::pair<std::vector<v2::ml_data_entry>, double> x_buffer_row_type;
  typedef std::vector<x_buffer_row_type> x_buffer_type;

  // These will be used by each thread
  std::vector<x_buffer_type> thread_x_buffers;

  std::vector<std::vector<std::pair<double, size_t> > > thread_candidate_pool;
  std::vector<std::vector<size_t> > thread_order_index_buffers;
  std::vector<dense_bitset> thread_negative_example_flags;
  std::vector<dense_bitset> thread_item_observed;

  /** The main method to do the explicit ranking stuff.
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
  std::pair<double, double> run_sgd_thread(
      size_t iteration,
      size_t thread_idx, size_t num_threads,
      size_t block_idx, size_t num_blocks,
      const v2::ml_data& data,
      SGDInterface* iface,
      double step_size,
      volatile bool& error_detected) GL_HOT {

    // Init an alternative random engine for shuffling things.  We
    // want the calculate_objective function to track the same calls
    // to the turicreate random number generator so that the exact
    // objective computed at the end is over the same pass of data
    // points as the one we track.  It's a small thing internally, but
    // important from a user experience perspective.

    std::default_random_engine shuffle_g(hash64(iteration, block_idx));

    static constexpr size_t ITEM_COLUMN_INDEX = 1;

    double loss_value = 0, rank_loss_value = 0;

    size_t n_items = data.metadata()->column_size(1);

    x_buffer_type& x_buffer = thread_x_buffers[thread_idx];
    dense_bitset& negative_example_flag = thread_negative_example_flags[thread_idx];

    size_t min_buffer_size = (11 * observation_block_size) / 10;
    if(x_buffer.size() < min_buffer_size) {

      // Give a little extra room to avoid expensive resizes
      x_buffer.resize(min_buffer_size);
      negative_example_flag.resize(min_buffer_size);
    }

    // Now, go through and extract everything from this block.
    auto it = data.get_block_iterator(block_idx, num_blocks);

    neg_sample_proc_buffer neg_exm_buffer;

    // A dense bitset for the number of observed items.
    dense_bitset& item_observed = thread_item_observed[thread_idx];
    item_observed.resize(n_items);
    item_observed.clear();

    // The vector of candidate examples.  This is what we use to
    // choose the negative example pool.

    std::vector<std::pair<double, size_t> >& candidate_pool = thread_candidate_pool[thread_idx];

    if(candidate_pool.size() != n_items)
      candidate_pool.resize(n_items);

    ////////////////////////////////////////////////////////////////////////////////
    // The main loop.

    while(!it.done() && !error_detected) {

      size_t n_items_in_buffer = 0;

      DASSERT_EQ(negative_example_flag.size(), x_buffer.size());
      DASSERT_TRUE(negative_example_flag.empty());

      // Fill up the buffer as much as possible.
      while(!it.done() && !error_detected && n_items_in_buffer < observation_block_size) {

        DASSERT_TRUE(item_observed.empty());

        size_t start_of_positive_examples = n_items_in_buffer;
        size_t n_taken_items = 0;

        size_t write_idx = start_of_positive_examples;

        while(!it.done()) {

          if(UNLIKELY(x_buffer.size() <= write_idx)) {
            auto resize = [&]() GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {
              size_t new_size = (5*(write_idx + 4)) / 4;
              x_buffer.resize(new_size);
              negative_example_flag.resize(new_size);
            };

            resize();
          }

          auto& x = x_buffer[write_idx].first;
          it.fill_observation(x);
          x_buffer[write_idx].second = it.target_value();
          negative_example_flag.clear_bit_unsync(write_idx);

          size_t item = x[ITEM_COLUMN_INDEX].index;
          DASSERT_LT(item, n_items);

          bool old_bit = item_observed.set_bit_unsync(item);

          // Account for the possibility there are duplicate items.
          if(!old_bit)
            ++n_taken_items;

          DASSERT_FALSE(x.empty());
          DASSERT_FALSE(x_buffer[write_idx].first.empty());

          ++write_idx;
          ++it;

          if(it.done() || it.is_start_of_new_block())
            break;
        }

        size_t n_rows = write_idx - start_of_positive_examples;

        n_items_in_buffer += n_rows;

        ////////////////////////////////////////////////////////////
        // Step 2.2: Loop through these rows, choosing one at each
        // point.

        // Don't add any negative examples to this part if the possible negative examples aren't there.
        if(UNLIKELY(n_taken_items == n_items)) {
          item_observed.clear();
          continue;
        }

        size_t target_num_negative_examples = n_rows;

        // Now attempt to fill up the buffer with candidate negative items
        size_t negative_sample_start_index = n_items_in_buffer;
        size_t current_write_idx = negative_sample_start_index;

        // Make sure we'll have enough room to add in the negative examples
        size_t required_x_buffer_size = current_write_idx + target_num_negative_examples;

        if(UNLIKELY(x_buffer.size() <= required_x_buffer_size)) {
          x_buffer.resize(required_x_buffer_size);
          negative_example_flag.resize(required_x_buffer_size);
        }

        // Now, for each positive example, select a number of
        // candidate negative examples.  Put these in the block [negative_sample_start_index, ...)
        for(size_t pos_idx = start_of_positive_examples;
            pos_idx < n_items_in_buffer && !error_detected;
            ++pos_idx) {

          // If there are no more items to sample
          if(n_taken_items == n_items)
            break;

          double score = this->choose_negative_example(
              thread_idx, data, iface,
              x_buffer[current_write_idx].first,
              x_buffer[pos_idx].first,
              item_observed,
              n_rows, n_items,
              n_taken_items,
              neg_exm_buffer);

          if(UNLIKELY(!std::isfinite(score))) {
            error_detected = true;
            break;
          }

          // Only use this item if it's acceptable.
          if(using_logistic_loss || score > unobserved_rating_value()) {
            negative_example_flag.set_bit_unsync(current_write_idx);
            ++n_taken_items;
            DASSERT_LT(current_write_idx, x_buffer.size());
            DASSERT_LT(ITEM_COLUMN_INDEX, x_buffer[current_write_idx].first.size());
            DASSERT_LT(x_buffer[current_write_idx].first[ITEM_COLUMN_INDEX].index, item_observed.size());

            item_observed.set_bit_unsync(x_buffer[current_write_idx].first[ITEM_COLUMN_INDEX].index);
            ++current_write_idx;
          }
        }

        if(UNLIKELY(error_detected))
          break;

        size_t num_negative_examples = current_write_idx - negative_sample_start_index;

        n_items_in_buffer = current_write_idx;

        // Clear out the points in the buffer.  It's important to do
        // this now, since the steps below will alter the negative examples.

        this->clear_item_observed_buffer(
            item_observed, n_rows + num_negative_examples, n_items,

            // This function maps from 0, ..., n_rows + num_candidate_items to the item index;
            [&](size_t i) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

              // indexes 0, ..., n_rows-1 are for the positive examples.
              return x_buffer[i + start_of_positive_examples].first[ITEM_COLUMN_INDEX].index;

            } );

        DASSERT_TRUE(item_observed.empty());


#ifndef NDEBUG
        for(size_t i = 0; i < n_items_in_buffer; ++i) {
          DASSERT_FALSE(x_buffer[i].first.empty());
        }
#endif

      } // End of while loop; loop while buffer has room or more data in block.

      if(UNLIKELY(error_detected))
        break;

      ////////////////////////////////////////////////////////////////////////////////
      // Part 2:  Now we have the buffer; run through things.

      DASSERT_LE(n_items_in_buffer, x_buffer.size());

      ////////////////////////////////////////////////////////////////////////////////
      // Part 2.1: Shuffle things via a random mapping so that we take
      // descents in random order.

      std::vector<size_t>& descent_order_indices = thread_order_index_buffers[thread_idx];
      descent_order_indices.resize(n_items_in_buffer);
      std::iota(descent_order_indices.begin(), descent_order_indices.end(), size_t(0));
      std::shuffle(descent_order_indices.begin(), descent_order_indices.end(), shuffle_g);

      ////////////////////////////////////////////////////////////////////////////////
      // Part 2.2: Now, go through and do a descent on each of these.

      for(size_t i = 0; i < n_items_in_buffer; ++i) {

        size_t index = descent_order_indices[i];
        DASSERT_LT(index, n_items_in_buffer);

        const auto& row = x_buffer[index];

        bool is_negative_example = negative_example_flag.get(index);

        double rr_scale = using_logistic_loss ? 1.0 : ranking_regularization;

        double r = (is_negative_example
                    ? unobserved_rating_value()
                    : row.second);

        double ss = (is_negative_example
                     ? step_size * rr_scale
                     : step_size);

        // Only apply the regularization descent operation if it's a
        // positive example.
        bool apply_regularization = !is_negative_example;

        DASSERT_FALSE(row.first.empty());

        double descent_loss = iface->apply_sgd_step(
            thread_idx, row.first, r, ss, apply_regularization);


        if(is_negative_example) {
          rank_loss_value += rr_scale * descent_loss;
        } else {
          loss_value += descent_loss;
        }
        // Test for numerical issues.

        if(!std::isfinite(loss_value + rank_loss_value)) {
          error_detected = true;
          break;
        }
      } // End loop over points in the buffer


      negative_example_flag.clear();

      // Further checks
      if(!iface->state_is_numerically_stable()) {
        error_detected = true;
      }
    }

    if(error_detected)
      return {NAN, NAN};
    else
      return {loss_value, rank_loss_value};
  }

  ////////////////////////////////////////////////////////////////////////////////

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
  std::pair<double, double> run_loss_calculation_thread(
      size_t thread_idx, size_t num_threads,
      const v2::ml_data& data,
      SGDInterface* iface) const {

    double loss_value = 0, rank_loss_value = 0;

    size_t n_items = data.metadata()->column_size(1);

    x_buffer_type x_buffer;

    // Start out with 4K possible items per user; doubles as needed.
    x_buffer.resize(4*1024);

    std::vector<v2::ml_data_entry> negative_example_x;

    neg_sample_proc_buffer neg_exm_buffer;

    dense_bitset item_observed(n_items);

    for(auto it = data.get_block_iterator(thread_idx, num_threads); !it.done();) {

      ////////////////////////////////////////////////////////////
      // Step 2.1: Fill up the buffer with potential positive
      // examples.
      size_t n_rows, n_rated_items;

      std::tie(n_rows, n_rated_items) =
          this->fill_x_buffer_with_users_items(
              x_buffer, it, n_items, item_observed);

      ////////////////////////////////////////////////////////////
      // Step 2.2: Loop through these rows, choosing one at each
      // point.

      if(n_rated_items == n_items) {

        ////////////////////////////////////////////////////////////
        // 2.2, case 1: All items have been rated.  If there are no
        // negative examples, then we just measure the fit to this
        // user.  This case is uncommon, but an important corner case.

        for(size_t i = 0; i < n_rows; ++i) {
          const std::vector<v2::ml_data_entry>& x = x_buffer[i].first;
          double y = x_buffer[i].second;

          double fx_hat = iface->calculate_fx(thread_idx, x);
          double loss = iface->loss_model.loss(fx_hat, y);

          ASSERT_GE(loss, 0);

          loss_value += loss;
        }

      } else {

        ////////////////////////////////////////////////////////////
        // 2.2, case 2: Not all items have been rated.  Sample
        // negative items at each stage, and score them.

        for(size_t i = 0; i < n_rows; ++i) {

          const std::vector<v2::ml_data_entry>& x = x_buffer[i].first;
          double y = x_buffer[i].second;

          DASSERT_GE(x.size(), 2);

          // Get the loss value from the positive example
          double fx_hat = iface->calculate_fx(thread_idx, x);
          loss_value += iface->loss_model.loss(fx_hat, y);

          // Choose a negative example.
          double negative_example_fx =
              this->choose_negative_example(
                  thread_idx,
                  data,
                  iface,
                  negative_example_x, x,
                  item_observed,
                  n_rows, n_items, n_rated_items,
                  neg_exm_buffer);

          if(!std::isfinite(negative_example_fx) || std::fabs(negative_example_fx) > 1e10) {
            return {NAN, NAN};
          }

          // Debug: Make sure the negative example isn't there
#ifndef NDEBUG
          for(size_t x_check = 0; x_check < n_rows; ++x_check) {
            DASSERT_NE(negative_example_x[1].index, x_buffer[x_check].first[1].index);
          }
#endif

          if(using_logistic_loss || negative_example_fx > unobserved_rating_value()) {

            double loss = iface->loss_model.loss(negative_example_fx, unobserved_rating_value());

            ASSERT_GE(loss, 0.0);

            double rr_scale = using_logistic_loss ? 1.0 : ranking_regularization;

            rank_loss_value += (rr_scale * loss);
          }

        } // End loop over points in the buffer
      }

      ////////////////////////////////////////////////////////////
      // Step 3.  Clear out the points in the buffer.

      this->clear_item_observed_buffer(item_observed, n_rows, n_items,
                                       [&](size_t i) { return x_buffer[i].first[1].index; } );
    }

    return {loss_value, rank_loss_value};
  }

};

}}

#endif
