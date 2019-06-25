/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_IMPLICIT_RANKING_SGD_SOLVER_CLASS_H_
#define TURI_SGD_IMPLICIT_RANKING_SGD_SOLVER_CLASS_H_

#include <map>
#include <vector>
#include <type_traits>
#include <core/util/code_optimization.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/factorization/ranking_sgd_solver_base.hpp>

namespace turi { namespace factorization {

/** Implicit Ranking
 *
 *  ============================================================
 *
 *  When the target is not present, do a gradient descent for all items
 *  such that the negative example is predicted more highly than the
 *  positive example.  Loss is defined as the number of out of order
 *  pairs.
 *
 */
template <class SGDInterface>
class implicit_ranking_sgd_solver final
    : public ranking_sgd_solver_base<SGDInterface>  {

 public:

  /**
   * Constructor.
   */
  implicit_ranking_sgd_solver(
      const std::shared_ptr<sgd::sgd_interface_base>& main_interface,
      const v2::ml_data& train_data,
      const std::map<std::string, flexible_type>& options)

      : ranking_sgd_solver_base<SGDInterface>(main_interface, train_data, options)
  {
    // Doesn't need to do anything here.
  }

 private:

  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Typedefs -- need to pull these here.

  typedef ranking_sgd_solver_base<SGDInterface> Base;
  typedef typename Base::x_buffer_row_type x_buffer_row_type;
  typedef typename Base::x_buffer_type x_buffer_type;
  typedef typename Base::neg_sample_proc_buffer neg_sample_proc_buffer;

  ////////////////////////////////////////////////////////////////////////////////

  /** The main method to do the implicit ranking stuff.
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

    double loss_value = 0;

    size_t n_items = data.metadata()->column_size(1);

    x_buffer_type x_buffer;

    // Start out with 4K possible items per user; doubles as needed.
    x_buffer.resize(4*1024);

    std::vector<v2::ml_data_entry> negative_example_x;

    neg_sample_proc_buffer neg_exm_buffer;

    dense_bitset item_observed(n_items);

    for(auto it = data.get_block_iterator(block_idx, num_blocks); !it.done() && !error_detected;) {

      ////////////////////////////////////////////////////////////
      // Step 2.1: Fill up the buffer with positive
      // examples.

      size_t n_rows, n_rated_items;

      std::tie(n_rows, n_rated_items) =
          this->fill_x_buffer_with_users_items(x_buffer, it, n_items, item_observed);

      // If there are no negatives, we ignore this user.  (The version
      // with a target column is handled differently).
      if(n_rated_items != n_items) {

        turi::random::shuffle(x_buffer.begin(), x_buffer.begin() + n_rows);

        ////////////////////////////////////////////////////////////
        // Step 2.2: Loop through these rows, choosing one positive
        // example and one negative example.

        for(size_t i = 0; i < n_rows; ++i) {

          const std::vector<v2::ml_data_entry>& x = x_buffer[i].first;

          double negative_example_fx =
              this->choose_negative_example(
                  thread_idx,
                  data,
                  iface,
                  negative_example_x, x,
                  item_observed,
                  n_rows, n_items, n_rated_items,
                  neg_exm_buffer);

          // Check to see if there was a numerical error; if so, reset
          // the model!
          if(!std::isfinite(negative_example_fx) || std::fabs(negative_example_fx) > 1e10) {
            error_detected = true;
            break;
          }

          // Check to make sure it's a true negative, not a false one.

#ifndef NDEBUG
          for(size_t x_check = 0; x_check < n_rows; ++x_check) {
            DASSERT_NE(negative_example_x[1].index, x_buffer[x_check].first[1].index);
          }
#endif

          double pw_loss_value = iface->apply_pairwise_sgd_step(
              thread_idx,
              x, negative_example_x,
              step_size);

          loss_value += pw_loss_value;

          if(!std::isfinite(loss_value) || pw_loss_value > 1e10) {
            error_detected = true;
            break;
          }

        } // End loop over points in the buffer
      }

      ////////////////////////////////////////////////////////////
      // Step 3.  Clear out the points in the buffer.

      this->clear_item_observed_buffer(item_observed, n_rows, n_items,
                                       [&](size_t i) { return x_buffer[i].first[1].index; } );
    }

    return {loss_value, loss_value};
  }


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
   *  rank_loss is the sampled weighted loss on the positive -
   *  negative examples.
   */
  std::pair<double, double> run_loss_calculation_thread(
      size_t thread_idx, size_t num_threads,
      const v2::ml_data& data,
      SGDInterface* iface) const {

    double loss_value = 0;

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
          this->fill_x_buffer_with_users_items(x_buffer, it, n_items, item_observed);

      // If there are no negatives, this user contributes nothing to
      // the count of out-of-order pairs.
      if(n_rated_items != n_items) {

        ////////////////////////////////////////////////////////////
        // Step 2.2: Loop through these rows, choosing one positive
        // example and one negative example.

        for(size_t i = 0; i < n_rows; ++i) {

          const std::vector<v2::ml_data_entry>& x = x_buffer[i].first;

          double positive_fx = iface->calculate_fx(thread_idx, x);

          double negative_example_fx =
              this->choose_negative_example(
                  thread_idx,
                  data,
                  iface,
                  negative_example_x, x,
                  item_observed,
                  n_rows, n_items, n_rated_items,
                  neg_exm_buffer);

          // If we've hit numerical errors
          if(!std::isfinite(negative_example_fx) || std::fabs(negative_example_fx) > 1e10) {
            return {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
          }

          // The loss_grad here applies to the difference
          loss_value += iface->loss_model.loss(positive_fx - negative_example_fx, 0);

        } // End loop over points in the buffer
      }

      ////////////////////////////////////////////////////////////
      // Step 3.  Clear out the points in the buffer.

      this->clear_item_observed_buffer(item_observed, n_rows, n_items,
                                       [&](size_t i) { return x_buffer[i].first[1].index; } );
    }

    return {loss_value, loss_value};
  }
};

}}

#endif
