/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_STARTUP_TEARDOWN_HPP
#define TURI_STARTUP_TEARDOWN_HPP
#include <core/export.hpp>
#include <string>

namespace turi {

/**
 * Configures the system global environment. This should be the first thing
 * (or close to the first thing) called on program startup.
 */
void EXPORT configure_global_environment(std::string argv0);

/**
 * This class centralizes all startup functions
 */
class EXPORT global_startup {
 public:
  global_startup() = default;

  global_startup(const global_startup&) = delete;
  global_startup(global_startup&&) = delete;
  global_startup& operator=(global_startup&&) = delete;
  global_startup& operator=(const global_startup&) = delete;

  /**
   * Performs all the startup calls immediately. Further calls to this
   * function does nothing.
   */
  void perform_startup();

  /**
   * Performs the startup if startup has not yet been performed.
   */
  ~global_startup();

  static global_startup& get_instance();

 private:
  bool startup_performed = false;
};

/**
 * This class centralizes all tear down functions allowing destruction
 * to happen in a prescribed order.
 *
 * TODO This can be more intelligent as required. For now, it is kinda dumb.
 */
class EXPORT global_teardown {
 public:
  global_teardown() = default;

  global_teardown(const global_teardown&) = delete;
  global_teardown(global_teardown&&) = delete;
  global_teardown& operator=(global_teardown&&) = delete;
  global_teardown& operator=(const global_teardown&) = delete;

  /**
   * Performs all the teardown calls immediately. Further calls to this
   * function does nothing.
   */
  void perform_teardown();

  /**
   * Performs the teardown if teardown has not yet been performed.
   */
  ~global_teardown();

  static global_teardown& get_instance();
 private:
  bool teardown_performed = false;
};

namespace teardown_impl {
extern EXPORT global_teardown teardown_instance;
}

} // turi
#endif
