/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/export.hpp>
#include <sstream>



extern "C" {
// Binary operators.  Available operations are:
// ==, !=, <=, <, >=, >, &, |, +, -, /, *, pow.
EXPORT tc_sarray* tc_binary_op_ss(const tc_sarray* sa1, const char* op,
                                  const tc_sarray* sa2, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "tc_sarray", NULL);
  CHECK_NOT_NULL(error, sa2, "tc_sarray", NULL);

  turi::gl_sarray ret = sa1->value.get_proxy()->vector_operator(sa2->value.get_proxy(), op);
  return new_tc_sarray(ret);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_binary_op_sf(const tc_sarray* sa1, const char* op,
                                  const tc_flexible_type* ft2,
                                  tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "tc_sarray", NULL);
  CHECK_NOT_NULL(error, ft2, "tc_flexible_type", NULL);

  turi::gl_sarray ret = sa1->value.get_proxy()->left_scalar_operator(ft2->value, op);
  return new_tc_sarray(ret);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_binary_op_fs(const tc_flexible_type* ft1, const char* op,
                           const tc_sarray* sa2, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft1, "tc_flexible_type", NULL);
  CHECK_NOT_NULL(error, sa2, "tc_sarray", NULL);

  turi::gl_sarray ret = sa2->value.get_proxy()->right_scalar_operator(ft1->value, op);

  return new_tc_sarray(ret);

  ERROR_HANDLE_END(error, NULL);
}
}

namespace turi {

/////////////////////////////////////////////////////////////////////////////
// Binary ops on flexible_type


/**  Binary operations on flexible type
 */
static inline flexible_type apply_binary_op_ff(
    const flexible_type& t, const char* op,
    const flexible_type& u) {

  enum class binary_op {
    OP_EQ = 0,
    OP_NEQ = 1,
    OP_LT = 2,
    OP_LE = 3,
    OP_GT = 4,
    OP_GE = 5,
    OP_BITWISE_AND = 6,
    OP_AND = 7,
    OP_BITWISE_OR = 8,
    OP_OR = 9,
    OP_PLUS = 10,
    OP_MINUS = 11,
    OP_MULT = 12,
    OP_DIV = 13,
    OP_MOD = 14
  };

  static std::map<std::string, binary_op> _op_map =  //
      {{"==", binary_op::OP_EQ},  {"!=", binary_op::OP_NEQ},
       {"<", binary_op::OP_LT},   {"<=", binary_op::OP_LE},
       {">", binary_op::OP_GT},   {">=", binary_op::OP_GE},
       {"&&", binary_op::OP_AND}, {"||", binary_op::OP_OR},
       {"+", binary_op::OP_PLUS}, {"-", binary_op::OP_MINUS},
       {"*", binary_op::OP_MULT}, {"/", binary_op::OP_DIV},
       {"%", binary_op::OP_MOD}};

  auto it = _op_map.find(op);
  if (it == _op_map.begin()) {
    std::ostringstream ss;
    ss << "Binary operator " << op << " not recognized. "
       << "Available operators are ";

    for (const auto& p : _op_map) {
      ss << p.first << " ";
    }
    ss << ".";

    throw std::invalid_argument(ss.str());
  }

  switch (it->second) {
    case binary_op::OP_EQ:
      return (t == u);
    case binary_op::OP_NEQ:
      return (t != u);
    case binary_op::OP_LT:
      return (t < u);
    case binary_op::OP_LE:
      return (t <= u);
    case binary_op::OP_GT:
      return (t > u);
    case binary_op::OP_GE:
      return (t <= u);
    case binary_op::OP_AND:
      return (t && u);
    case binary_op::OP_OR:
      return (t || u);
    case binary_op::OP_PLUS:
      return (t + u);
    case binary_op::OP_MINUS:
      return (t - u);
    case binary_op::OP_MULT:
      return (t * u);
    case binary_op::OP_DIV:
      return (t / u);
    case binary_op::OP_MOD:
      return (t % u);
    default:
      return FLEX_UNDEFINED;
  }
}
}

extern "C" {

EXPORT tc_flexible_type* tc_binary_op_ff(const tc_flexible_type* ft1,
                                         const char* op,
                                         const tc_flexible_type* ft2,
                                         tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft1, "tc_flexible_type", NULL);
  CHECK_NOT_NULL(error, ft2, "tc_flexible_type", NULL);

  return new_tc_flexible_type(apply_binary_op_ff(ft1->value, op, ft2->value));

  ERROR_HANDLE_END(error, NULL);
}

}

/////////////////////////////////////////////////////////////////////////////
// Unary Ops


namespace turi {

enum class unary_op {
  OP_NEGATE = 0,
  OP_ABS = 1,
  OP_NOT = 2
};

/**  Binary operations
 */
template <typename T>
static inline T apply_unary_op(const T& t, const char* op) {

  static std::map<std::string, unary_op> _op_map = //
    { {"-", unary_op::OP_NEGATE},
      {"abs", unary_op::OP_ABS},
      {"!", unary_op::OP_NOT} };

  auto it = _op_map.find(op);
  if(it == _op_map.begin()) {
    std::ostringstream ss;
    ss << "Unary operator " << op << " not recognized. "
       << "Available operators are ";

    for(const auto& p : _op_map) {
      ss << p.first << " ";
    }
    ss << ".";

    throw std::invalid_argument(ss.str());
  }

  switch (it->second) {
    case unary_op::OP_NEGATE:
      return (0 - t);
//    case unary_op::OP_ABS:
//      return (std::abs(t));
    case unary_op::OP_NOT:
      return (t == 0);
    default:
      return t;
  }
}
}

extern "C" {

// Unary operators -- a collection of predefined unary operators that
// perform optimized one-to-one unary operations on the models.
EXPORT tc_flexible_type* tc_ft_unary_op(const tc_flexible_type* ft1,
                                        const char* op, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft1, "tc_flexible_type", NULL);

  return new_tc_flexible_type(turi::apply_unary_op(ft1->value, op));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_unary_op(const tc_sarray* sa1, const char* op, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "tc_sarray", NULL);

  return new_tc_sarray(turi::apply_unary_op(sa1->value, op));

  ERROR_HANDLE_END(error, NULL);
}


}
