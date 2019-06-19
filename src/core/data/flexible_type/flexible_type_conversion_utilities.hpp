/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_FLEXIBLE_TYPE_CONVERSION_UTILS_HPP
#define TURI_FLEXIBLE_TYPE_FLEXIBLE_TYPE_CONVERSION_UTILS_HPP

#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <type_traits>
#include <core/util/code_optimization.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/flexible_type/type_traits.hpp>

namespace turi {

namespace flexible_type_internals {

class type_conversion_error : public std::runtime_error {
  public:
    explicit type_conversion_error(const std::string& message);
    explicit type_conversion_error(const char* message);
};

void throw_type_conversion_error(const flexible_type& val, const char *type);

template <typename Arg>
static void __unpack_args(std::ostringstream& ss, Arg a) {
  ss << a;
}

template <typename Arg, typename... OtherArgs>
static void __unpack_args(std::ostringstream& ss, Arg a1, OtherArgs... a2) {
  ss << a1;
  __unpack_args(ss, a2...);
}

template <typename... PrintArgs>
static void throw_type_conversion_error(const flexible_type& val, const char* type, PrintArgs... args) {
  std::ostringstream ss;
  ss << "Type conversion failure in flexible_type converter: expected " << type;
  __unpack_args(ss, args...);
  ss << "; got " << flex_type_enum_to_name(val.get_type());

  throw type_conversion_error(ss.str());
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
static void __get_t(flexible_type& dest, const T& src) {
  convert_to_flexible_type(dest, src);
}

template <typename T>
static void __get_t(double& dest, const T& src) {
  dest = src;
}

template <size_t idx, typename C, typename... Args>
void __unpack_tuple(C& dest, const std::tuple<Args...>& src,
                    typename std::enable_if<idx == sizeof...(Args)>::type* = NULL) {}

template <size_t idx, typename C, typename... Args>
void __unpack_tuple(C& dest, const std::tuple<Args...>& src,
                    typename std::enable_if<idx != sizeof...(Args)>::type* = NULL) {
   __get_t(dest[idx], std::get<idx>(src));
   __unpack_tuple<idx + 1>(dest, src);
}

template <typename C, typename... Args>
static void unpack_tuple(C& dest, const std::tuple<Args...>& src) {
   __unpack_tuple<0>(dest, src);
}

////////////////////////////////////////////////////////////////////////////////


template <typename T>
static void __set_t(T& dest, const flexible_type& src) {
  convert_from_flexible_type(dest, src);
}

template <typename T>
static void __set_t(T& dest, double src) {
  dest = static_cast<T>(src);
}

template <size_t idx, typename C, typename... Args>
void __pack_tuple(std::tuple<Args...>& dest, const C& src,
                    typename std::enable_if<idx == sizeof...(Args)>::type* = NULL) {}

template <size_t idx, typename C, typename... Args>
void __pack_tuple(std::tuple<Args...>& dest, const C& src,
                    typename std::enable_if<idx != sizeof...(Args)>::type* = NULL) {
   __set_t(std::get<idx>(dest), src[idx]);
   __pack_tuple<idx + 1>(dest, src);
}

template <typename C, typename... Args>
static void pack_tuple(std::tuple<Args...>& dest, const C& src) {
   __pack_tuple<0>(dest, src);
}

}}


#endif
