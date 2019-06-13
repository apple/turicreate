/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_COMMON_ERROR_TYPES_HPP
#define CPPIPC_COMMON_ERROR_TYPES_HPP
#include <string>
#include <exception>
#include <typeinfo>
#include <core/export.hpp>
namespace turi {

/**
 * Subclass the std::bad_alloc with custom message.
 */
class EXPORT bad_alloc : public std::bad_alloc {

  std::string msg;

  public:
    bad_alloc(const std::string& msg) : msg(msg) {}
    virtual const char* what() const throw() {
      return msg.c_str();
    }
};

/**
 * Subclass the std::bad_cast with custom message.
 */
class EXPORT bad_cast : public std::bad_cast {

  std::string msg;

  public:
  bad_cast(const std::string& msg) : msg(msg) {}
  virtual const char* what() const throw() {
    return msg.c_str();
  }
};

} // cppipc
#endif
