/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History:

// 27 Feb 2001   Jeremy Siek
//      Initial checkin.

#ifndef TURI_FUNCTION_OUTPUT_ITERATOR_HPP
#define TURI_FUNCTION_OUTPUT_ITERATOR_HPP

#include <iterator>

namespace turi {

  /**
   * \ingroup util
   * Copied and amended from boost.
   * Applies a function to an iterator, outputing an iterator.
   */
  template <class UnaryFunction, class MoveFunction>
  class function_output_iterator {
    typedef function_output_iterator self;
  public:
    typedef std::output_iterator_tag iterator_category;
    typedef void                value_type;
    typedef void                difference_type;
    typedef void                pointer;
    typedef void                reference;

    explicit function_output_iterator() {}

    explicit function_output_iterator(const UnaryFunction& f, const MoveFunction& f2)
      : m_f(f), m_f2(f2) {}

    struct output_proxy {
      output_proxy(UnaryFunction& f, MoveFunction& f2) : m_f(f), m_f2(f2) { }
      template <class T> output_proxy& operator=(const T& value) {
        m_f(value);
        return *this;
      }


      template <typename T>
      void output_forward(T&& value, std::integral_constant<bool, true>) {
        // value is an rvalue
        m_f2(std::move(value));
      }

      template <typename T>
      void output_forward(T&& value, std::integral_constant<bool, false>) {
        // value is an lvalue
        m_f(value);
      }

      template <class T> output_proxy& operator=(T&& value) {
        // annoying T&& is a universal reference.
        // So redispatch with a check for r-value-referenceness
        // and call the move version or the const ref version as required
        output_forward(std::forward<T>(value),
                       typename std::is_rvalue_reference<decltype(std::forward<T>(value))>::type());
        return *this;
      }

      template <class T> output_proxy& operator=(const T&& value) {
        // overload for const T condition. in which case we call the
        // const T& version
        output_forward(value, std::integral_constant<bool, false>());
        return *this;
      }


      UnaryFunction& m_f;
      MoveFunction& m_f2;
    };
    output_proxy operator*() { return output_proxy(m_f, m_f2); }
    self& operator++() { return *this; }
    self& operator++(int) { return *this; }
  private:
    UnaryFunction m_f;
    MoveFunction m_f2;
  };
} // namespace boost

#endif // BOOST_FUNCTION_OUTPUT_ITERATOR_HPP
