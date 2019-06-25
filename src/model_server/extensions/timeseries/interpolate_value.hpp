/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef RESAMPLE_INTERPOLATE_VALUE_H
#define RESAMPLE_INTERPOLATE_VALUE_H

#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {
namespace timeseries {

/**
 * Simple interface for 2-D interpolation required for re-sample.
 * functions.
 *
 * \code
 * output = interpolator.emit(1.5, 1, 1, 2, 2)
 * \endcode
 *
 * As an example, consider the following simple function which interpolates
 * values linearly.
 *
 * Interpolates the value at t, using the values at (t1, v1), (t2, v2)
 * \code
    linear = [](const flexible_type& t,
        const flexible_type& t1, const flexible_type& t2,
        const flexible_type& v1, const flexible_type& v2) {
      return v1 + (v2 - v2) * (t - t1) / (t2 - t1);
    }
 *
 * \endcode
 */

class interpolator_value {

 public:
  /**
   * Returns true if the the aggregate_value can consume a column of this type,
   * and false otherwise. (For instance, a sum aggregator can consume integers
   * and floats, and not anything else).
   */
  virtual bool support_type(flex_type_enum type) const = 0;

  /**
   * Sets the input types and returns the output type.
   *
   * Default implementation assumes there is ony one input, and output
   * type is the same as input type.
   */
  virtual flex_type_enum set_input_types(const std::vector<flex_type_enum>&
      types) {
    DASSERT_TRUE(types.size() == 1);
    return set_input_type(types[0]);
  }

  /**
   * Returns a printable name of the operation.
   */
  virtual std::string name() const = 0;

  /**
   * Interpolate the value at t, given the (t1, v1) and (t2, v2)
   */
  virtual flexible_type interpolate(
    const flexible_type& t,  const flexible_type& t1, const flexible_type& t2,
    const flexible_type& v1, const flexible_type& v2) const = 0;

 protected:
  virtual flex_type_enum set_input_type(flex_type_enum type) {
     return type;
  }
};

class zero_fill : public interpolator_value {
 public:

  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER) ||
           (type == flex_type_enum::FLOAT) ||
           (type == flex_type_enum::VECTOR);
  }

  std::string name() const {
    return std::string("zero");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    if (v1.get_type() == flex_type_enum::VECTOR ||
        v2.get_type() == flex_type_enum::VECTOR) {
      // First or last may be None.
      if (v1.get_type() == flex_type_enum::VECTOR) {
        return flexible_type(flex_vec(v1.size(), 0));
      } else {
        return flexible_type(flex_vec(v2.size(), 0));
      }
    } else if (((v1.get_type() == flex_type_enum::INTEGER) ||
                 (v1.get_type() == flex_type_enum::FLOAT))  &&
        ((v2.get_type() == flex_type_enum::INTEGER) ||
                 (v2.get_type() == flex_type_enum::FLOAT))) {
      return flexible_type(0);
    } else {
      DASSERT_TRUE(false);
    }
  }
};

class forward_fill: public interpolator_value {

 public:

  bool support_type(flex_type_enum type) const {
    return true;
  }

  std::string name() const {
    return std::string("ffill");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    return t1 <= t2 ? v1 : v2;
  }
};

class nearest : public interpolator_value {

 public:

  bool support_type(flex_type_enum type) const {
    return true;
  }

  std::string name() const {
    return std::string("nearest");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    size_t del_t1 = t.get<flex_date_time>().posix_timestamp()
                        - t1.get<flex_date_time>().posix_timestamp();
    size_t del_t2 = t2.get<flex_date_time>().posix_timestamp()
                        -  t.get<flex_date_time>().posix_timestamp();
    return (del_t1 <= del_t2) ? v1 : v2;
  }
};

class backward_fill : public interpolator_value {

 public:

  bool support_type(flex_type_enum type) const {
    return true;
  }

  std::string name() const {
    return std::string("bfill");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    return (t2 >= t1) ? v2 : v1;
  }
};

class none_fill: public interpolator_value {

 public:

  bool support_type(flex_type_enum type) const {
    return true;
  }

  std::string name() const {
    return std::string("none");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    return FLEX_UNDEFINED;
  }
};

class linear_interpolation : public interpolator_value {

 public:

  bool support_type(flex_type_enum type) const {
    return (type == flex_type_enum::INTEGER) ||
           (type == flex_type_enum::FLOAT) ||
           (type == flex_type_enum::VECTOR);
  }

  std::string name() const {
    return std::string("linear");
  }

  flexible_type interpolate(const flexible_type& t,  const flexible_type& t1,
                            const flexible_type& t2, const flexible_type& v1,
                            const flexible_type& v2) const {
    if ((((v1.get_type() == flex_type_enum::INTEGER) ||
               (v1.get_type() == flex_type_enum::FLOAT))  &&
         ((v2.get_type() == flex_type_enum::INTEGER) ||
                (v2.get_type() == flex_type_enum::FLOAT))) ||
       ((v1.get_type() == flex_type_enum::VECTOR) &&
          (v2.get_type() == flex_type_enum::VECTOR))) {
     const auto& _t  =  t.get<flex_date_time>();
     const auto& _t1 = t1.get<flex_date_time>();
     const auto& _t2 = t2.get<flex_date_time>();
     int dt_t1 = _t.posix_timestamp()  - _t1.posix_timestamp();
     int dt_21 = _t2.posix_timestamp() - _t1.posix_timestamp();
     flexible_type scale = double(dt_t1) / dt_21;
     if (v1.get_type() == flex_type_enum::INTEGER) {
       return (dt_21 == 0) ? v1: scale * (v2 - v1) + v1;  // flex_float
     } else {
       return (dt_21 == 0) ? v1: v1 + (v2 - v1) * scale;  // type(v1)
     }
   }
  }

  flex_type_enum set_input_type(flex_type_enum type) {
    if ((type == flex_type_enum::INTEGER) || (type == flex_type_enum::FLOAT)) {
      return flex_type_enum::FLOAT;
    } else if (type == flex_type_enum::VECTOR) {
      return flex_type_enum::VECTOR;
    } else {
      DASSERT_TRUE(false);
    }
  }
};


/**
 * Helper function to convert string interpolation operators to the built-in
 * functions.
 */
EXPORT inline std::shared_ptr<interpolator_value> get_builtin_interpolator(const
    std::string& fill_method) {
  std::shared_ptr<interpolator_value> interpolation_fn;
  if (fill_method == "__builtin__zero__") {
    interpolation_fn.reset(new zero_fill());
  } else if (fill_method == "__builtin__nearest__") {
    interpolation_fn.reset(new nearest());
  } else if (fill_method == "__builtin__ffill__") {
    interpolation_fn.reset(new forward_fill());
  } else if (fill_method == "__builtin__bfill__") {
    interpolation_fn.reset(new backward_fill());
  } else if (fill_method == "__builtin__none__") {
    interpolation_fn.reset(new none_fill());
  } else if (fill_method == "__builtin__linear__") {
    interpolation_fn.reset(new linear_interpolation());
  } else {
    log_and_throw("Internal error. Undefined interpolation method.");
  }
  return interpolation_fn;
}

} // timeseries
} // turicreate

#endif
