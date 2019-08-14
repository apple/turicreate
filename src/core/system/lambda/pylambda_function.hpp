/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_PYLAMBDA_FUNCTION_HPP
#define TURI_LAMBDA_PYLAMBDA_FUNCTION_HPP

#include<vector>
#include<string>
#include<core/data/flexible_type/flexible_type.hpp>

namespace turi {

class sframe_rows;

namespace fileio {
struct file_ownership_handle;
}

namespace lambda {

/**
 * Represents a python lambda function object which is evaluated in parallel.
 *
 * Usage
 * -----
 * \code
 *
 * // Constructed from a pickled lambda string.
 * std::string lambda_string="some pickled str"
 * pylambda_function f(lambda_string);
 *
 * // (optional) Set options such as random_seed or skip_undefined.
 * f.set_skip_undefined(true);
 * f.set_random_seed(0);
 *
 * // evaluate on a minibatch of values.
 * std::vector<flexible_type> out;
 * f.evaluate({1,2,3}, out);
 *
 * \endcode
 *
 * The pylambda_function can also contain a gl_pickle directory.
 * The gl_pickle must contain one function.
 * The function will then be unpickled from the directory and used.
 * \code
 * // Constructed from a pickled lambda string.
 * std::string gl_pickle_directory = "./pickled_function"
 * pylambda_function f(gl_pickle_directory);
 * \endcode
 *
 * The evaluation is implemented using pylambda master and workers
 * for parallelism:
 *  - each call to evaluate() will grab one avaiable worker.
 *  - if no worker is avaiable, block.
 *  - when evaluate returns, the corresponding worker is released.
 */
class pylambda_function {
 public:
  /**
   * Constructs a lambda function from a either a series of pickled bytes
   * or a gl_pickled directory which contains a function.
   * \param delete_pickle_files_on_destruction Only meaningful if
   * lambda_str contains a gl_pickled directory. If true (default), it
   * the directory will be deleted when this pylambda_function instance is
   * destroyed.
   */
  pylambda_function(const std::string& lambda_str,
                    bool delete_pickle_files_on_destruction = true);

  pylambda_function(const pylambda_function& other) = delete;
  pylambda_function& operator=(const pylambda_function& other) = delete;

  ~pylambda_function();

  //// Options
  void set_skip_undefined(bool value);
  void set_random_seed(int value);

  //// Evaluating Interface

  /* One to one */
  void eval(const sframe_rows& rows,
            std::vector<flexible_type>& out);

  /* Many to one */
  void eval(const std::vector<std::string>& keys,
            const sframe_rows& rows,
            std::vector<flexible_type>& out);

 private:
  size_t lambda_hash = -1;
  bool skip_undefined = false;
  size_t random_seed = 0;
  std::shared_ptr<fileio::file_ownership_handle> m_pickle_file_handle;
};

  } // end of lambda namespace
} // end of turi namespace

#endif
