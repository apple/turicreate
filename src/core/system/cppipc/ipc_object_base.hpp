/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_IPC_OBJECT_BASE_HPP
#define CPPIPC_IPC_OBJECT_BASE_HPP
#include <memory>
#include <core/export.hpp>
/**
 * All exported base classes must inherit from this class.
 */
namespace cppipc {

class EXPORT ipc_object_base: public std::enable_shared_from_this<ipc_object_base> {
 public:
  virtual ~ipc_object_base();
};

} // cppipc



#endif
