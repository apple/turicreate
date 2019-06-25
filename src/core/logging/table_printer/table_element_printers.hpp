/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TABLE_ELEMENT_PRINTERS_H_
#define TURI_TABLE_ELEMENT_PRINTERS_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <type_traits>
#include <atomic>
#include <core/parallel/atomic.hpp>

namespace turi {

struct progress_time;

namespace table_internal {

/** Printers for each of the primimtive types.  Called by the routines below.
 */

void _print_string(std::ostringstream& ss, size_t width, const std::string& s);
void _print_double(std::ostringstream& ss, size_t width, double s);
void _print_bool(std::ostringstream& ss, size_t width, bool b);
void _print_long(std::ostringstream& ss, size_t width, long v);
void _print_time(std::ostringstream& ss, size_t width, double pt);
void _print_flexible_type(std::ostringstream& ss, size_t width, const flexible_type& pt);

////////////////////////////////////////////////////////////////////////////////
// Now specific classes to direct how each type is printed and how
// values are stored in the type string.

class table_printer_element_base {
 public:
  enum class style_type : uint8_t {
    kDefault,       // Written as number or string
    kBool,          // Written as true or false
    kProgressTime,  // Written as human-readable elapsed time
  };

  virtual void print(std::ostringstream& ss, size_t width){};
  virtual flexible_type get_value(){return FLEX_UNDEFINED;}
};

template <typename T, class Enable = void>
struct table_printer_element : public table_printer_element_base
{
  static constexpr bool valid_type = false;
  static constexpr style_type style = style_type::kDefault;
};

/** For printing doubles.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if<std::is_floating_point<T>::value>::type >
    : public table_printer_element_base {

 public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kDefault;

  table_printer_element(T v)
      : value(double(v))
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_double(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  double value;
};



/** For printing bools.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if<std::is_same<T, bool>::value>::type >
    : public table_printer_element_base {
 public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kBool;

  table_printer_element(T v)
      : value(v)
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_bool(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  bool value;
};


/** For printing signed integers.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if< (std::is_integral<T>::value && (!std::is_same<T, bool>::value))>::type >
    : public table_printer_element_base {
public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kDefault;

  table_printer_element(const T& v)
      : value(v)
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_long(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  long value;
};

/** For printing std atomics.
 */
template <typename T>
struct table_printer_element
<std::atomic<T>, typename std::enable_if<std::is_integral<T>::value>::type >
    : public table_printer_element<T> {
public:
  table_printer_element(const std::atomic<T>& v)
      : table_printer_element<T>(T(v))
  {}
};

/** For printing turicreate atomics.
 */
template <typename T>
struct table_printer_element
<turi::atomic<T>, typename std::enable_if<std::is_integral<T>::value>::type >
    : public table_printer_element<T> {
public:
  table_printer_element(const std::atomic<T>& v)
      : table_printer_element<T>(T(v))
  {}
};


/** For printing strings.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if<std::is_convertible<T, std::string>::value
                            && !std::is_same<T, flexible_type>::value>::type>
    : public table_printer_element_base {

 public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kDefault;

  table_printer_element(const T& v)
      : value(v)
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_string(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  std::string value;
};

/** For printing progress time.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if<std::is_same<T, progress_time>::value>::type>
    : public table_printer_element_base {

 public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kProgressTime;

  table_printer_element(double v)
      : value(v)
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_time(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  double value;
};

/** For printing flexible_type.
 */
template <typename T>
struct table_printer_element
<T, typename std::enable_if<std::is_same<T, flexible_type>::value>::type >
    : public table_printer_element_base {
 public:
  static constexpr bool valid_type = true;
  static constexpr style_type style = style_type::kDefault;

  table_printer_element(const T& v)
      : value(v)
  {}

  void print(std::ostringstream& ss, size_t width) {
    _print_flexible_type(ss, width, value);
  }

  flexible_type get_value() {
    return value;
  }

private:
  flexible_type value;
};


}}

#endif /* TURI_TABLE_PRINTER_IMPL_H_ */
