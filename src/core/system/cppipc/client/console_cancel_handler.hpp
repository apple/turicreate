/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_HPP
#define CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_HPP
#include <atomic>
#include <iostream>
#include <core/export.hpp>
/**
 * \ingroup cppipc
 *
 * Singleton object that sets the system-specific handler of a
 * "cancel" event from a console application.  This could be Ctrl+C (UNIX and
 * Windows), Ctrl+Break (just Windows), and whatever some other system
 * supports. This abstraction handles ONLY cancel events and no other signals,
 * as it is much more difficult to provide a cross-platform signal handling
 * method.
 */

namespace cppipc {

class EXPORT console_cancel_handler {
 public:
  static console_cancel_handler& get_instance();

  // Guarantees that if this returns false, new handler wasn't set
  // TODO: Check that statement
  // TODO: Get this to take an arbitrary function
  virtual bool set_handler() { return false; }

  // If this fails, your signal handler could be in a weird state...m_sigint_act
  // will be set still. Sorry!
  virtual bool unset_handler() { return false; }

  virtual void raise_cancel() {}

  bool get_cancel_flag() {
    return m_cancel_on.load();
  }

  void set_cancel_flag(bool val) {
    m_cancel_on.store(val);
  }

 protected:
  console_cancel_handler() {
    this->set_cancel_flag(false);
  }

  std::atomic<bool> m_cancel_on;

  bool m_handler_installed = false;

 private:
  console_cancel_handler(console_cancel_handler const&) = delete;

  console_cancel_handler& operator=(console_cancel_handler const&) = delete;

};

} // namespace cppipc

#endif //CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_HPP
