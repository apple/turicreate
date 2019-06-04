/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_BASIC_SGD_SOLVER_CLASS_H_
#define TURI_SGD_BASIC_SGD_SOLVER_CLASS_H_

#include <map>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/sgd/sgd_solver_base.hpp>
#include <toolkits/sgd/sgd_interface.hpp>

#ifdef interface
#undef interface
#endif

namespace turi {

class option_manager;

namespace sgd {

template <class SGDInterface>
class basic_sgd_solver : public sgd_solver_base {
 private:
  const size_t max_num_threads = 0;

  // Memory to hold things across threads.
  std::vector<std::vector<std::pair<std::vector<v2::ml_data_entry>, double> > > x_buffers;

 public:

  /** Constructor.
   *
   *  Construct the sgd solver.
   */
  basic_sgd_solver(const std::shared_ptr<sgd_interface_base>& interface,
                   const v2::ml_data& train_data,
                   const std::map<std::string, flexible_type>& options)

      : sgd_solver_base(interface, train_data, options)
      , max_num_threads(thread::cpu_count())
  {
    x_buffers.resize(max_num_threads);
  }

  /** Call the following function to insert the option definitions
   *  needed for the basic sgd solver into an option manager.
   */
  static void add_options(option_manager& options) {

    // No additional options beyond those in the base sgd solver
    // class.
    sgd_solver_base::add_options(options);
  }



  /** Run a single SGD pass through the data.
   *
   */
  std::pair<double, double> run_iteration(
      size_t iteration,
      sgd_interface_base* iface_base,
      const v2::ml_data& data,
      double step_size) GL_HOT {

    ////////////////////////////////////////////////////////////////////////////////
    // Set up a few preliminary variables

    SGDInterface* iface = dynamic_cast<SGDInterface*>(iface_base);

    volatile bool error_detected = false;

    std::vector<double> loss_values(max_num_threads, 0);

    iface->setup_iteration(iteration, step_size);

    ////////////////////////////////////////////////////////////////////////////////
    // Do one parallel pass through the data.  Randomize as much as possible

    // Slice up the initial input data, so we take it from different
    // sections each time.  Since ml_data has a block cache manager
    // and other
    size_t num_blocks = 16*max_num_threads;

    std::vector<size_t> blocks_to_use(num_blocks);
    std::iota(blocks_to_use.begin(), blocks_to_use.end(), 0);
    random::shuffle(blocks_to_use);

    atomic<size_t> current_block = 0;

    size_t block_size = options.at("sgd_sampling_block_size");

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

        // Get the buffers
        std::vector<std::pair<std::vector<v2::ml_data_entry>, double> >& x_buffer = x_buffers[thread_idx];
        x_buffer.resize(block_size);

        loss_values[thread_idx] = 0;

        while(!error_detected) {
          size_t block_lookup_idx = (++current_block) - 1;

          // we're done in this case.
          if(block_lookup_idx >= num_blocks)
            break;

          size_t block = blocks_to_use[block_lookup_idx];

          // Get the iterator for this.
          auto it = data.get_iterator(block, num_blocks);

          while(!error_detected && !it.done()) {
            // Fill up the buffer

            size_t idx = 0;

            for(idx = 0; idx < block_size && !it.done(); ++idx, ++it) {
              it.fill_observation(x_buffer[idx].first);
              x_buffer[idx].second = it.target_value();
            }

            size_t n = idx;

            // Shuffle things!
            random::shuffle(x_buffer.begin(), x_buffer.begin() + n);

            for(size_t i = 0; i < n && !error_detected; ++i) {
              std::vector<v2::ml_data_entry>& x = x_buffer[i].first;
              double y = x_buffer[i].second;

              double current_loss_value = iface->apply_sgd_step(thread_idx, x, y, step_size);

              ////////////////////////////////////////////////////////////
              // Do a gradient step.  The loss value is the one at the
              // current point, before the sgd step is performed.

              loss_values[thread_idx] += current_loss_value;

              if(!std::isfinite(loss_values[thread_idx]) ) {
                logstream(LOG_INFO) << "SGD: Non-finite loss value in thread " << thread_idx << std::endl;
                error_detected = true;
              }
            }
          }
        }
      }
      );

    if(error_detected)
      return {NAN, NAN};

    // Finalize the iteration.
    iface->finalize_iteration();

    double loss_no_regularization
        = (std::accumulate(loss_values.begin(), loss_values.end(), double(0.0))
           / std::max(size_t(1), data.size()));

    double regularization_penalty = iface->current_regularization_penalty();

    double objective_value_estimate = loss_no_regularization + regularization_penalty;

    // Is it a trivial model?  If so, we can break early.
    if(objective_value_estimate <= 1e-16)
      return {0,0};

    double reported_training_loss = iface->reported_loss_value(loss_no_regularization);

    return {objective_value_estimate, reported_training_loss};
  }


};

}}

#endif
