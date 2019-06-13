/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_GRAPH_PYLAMBDA_MASTER_HPP
#define TURI_LAMBDA_GRAPH_PYLAMBDA_MASTER_HPP

#include<core/system/lambda/graph_lambda_interface.hpp>
#include<core/system/lambda/worker_pool.hpp>

namespace turi {

namespace lambda {
  /**
   * \ingroup lambda
   *
   * Simple singleton object managing a worker_pool of graph lambda workers.
   */
  class graph_pylambda_master {
   public:

    static graph_pylambda_master& get_instance();

    static void shutdown_instance();

    graph_pylambda_master(size_t nworkers = 8);

    inline size_t num_workers() { return m_worker_pool->num_workers(); }

    static void set_pylambda_worker_binary(const std::string& path) { pylambda_worker_binary = path; };

    inline std::shared_ptr<worker_pool<graph_lambda_evaluator_proxy>> get_worker_pool() {
      return m_worker_pool;
    }

   private:

    graph_pylambda_master(graph_pylambda_master const&) = delete;

    graph_pylambda_master& operator=(graph_pylambda_master const&) = delete;

   private:
    std::shared_ptr<worker_pool<graph_lambda_evaluator_proxy>> m_worker_pool;

    static std::string pylambda_worker_binary;
  };
} // end lambda
} // end turicreate

#endif
