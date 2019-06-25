/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_WIN_HPP
#define CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_WIN_HPP

#include <core/system/cppipc/client/console_cancel_handler.hpp>

namespace cppipc {

class console_cancel_handler_win : public console_cancel_handler {
 public:
  typedef console_cancel_handler super;

  bool set_handler();
  bool unset_handler();
  void raise_cancel();
  console_cancel_handler_win() : super() {}
 private:

};
} // namespace cppipc


#endif //CPPIPC_SERVER_CONSOLE_CANCEL_HANDLER_WIN_HPP
