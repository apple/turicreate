/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <cmath>
#include <functional>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_interface/unity_sarray_binary_operations.hpp>

namespace turi {
namespace unity_sarray_binary_operations {
void check_operation_feasibility(flex_type_enum left,
                                 flex_type_enum right,
                                 std::string op) {
  bool operation_is_feasible = false;

  if (op == "+" || op == "-" || op == "*" || op == "/") {
    if (left == flex_type_enum::DATETIME && right == flex_type_enum::DATETIME && op == "-") {
      operation_is_feasible = true;
    } else {
      operation_is_feasible = flex_type_has_binary_op(left, right, op[0]) ||
                            ((left == flex_type_enum::FLOAT
                             || left == flex_type_enum::INTEGER
                             || left == flex_type_enum::VECTOR)
                             && (right == flex_type_enum::FLOAT
                                 || right == flex_type_enum::INTEGER
                                 || right == flex_type_enum::VECTOR)) ||
                            ((left == flex_type_enum::FLOAT
                             || left == flex_type_enum::INTEGER
                             || left == flex_type_enum::ND_VECTOR)
                             && (right == flex_type_enum::FLOAT
                                 || right == flex_type_enum::INTEGER
                                 || right == flex_type_enum::ND_VECTOR));
    }
  } else if (op == "%") {
    operation_is_feasible = left == flex_type_enum::INTEGER &&
                            right == flex_type_enum::INTEGER;
  } else if (op == "**" || op == "//") {
    operation_is_feasible = (
        (left == flex_type_enum::FLOAT
         || left == flex_type_enum::INTEGER
         || left == flex_type_enum::VECTOR)
         && (right == flex_type_enum::FLOAT
             || right == flex_type_enum::INTEGER
             || right == flex_type_enum::VECTOR));
  } else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
    // the comparison operators are all compatible. we just need to check
    // the < operator
    operation_is_feasible = flex_type_has_binary_op(left, right, '<');
  } else if (op == "==" || op == "!=") {
    // equality comparison is always feasible
    operation_is_feasible = true;
  } else if (op == "&" || op == "|") {
    // boolean operations are always feasible
    operation_is_feasible = true;
  } else if (op == "in") {
    // note that while the op is "in", the direction of the operator is
    // [BIGGER_LIST "in" element] rather than ["element" in BIGGER_LIST]
    // yes. slightly disconcerting I know. Sorry.
    operation_is_feasible = (left == flex_type_enum::STRING && right == flex_type_enum::STRING) ||
                            (left == flex_type_enum::VECTOR && right == flex_type_enum::FLOAT) ||
                            (left == flex_type_enum::VECTOR && right == flex_type_enum::INTEGER) ||
                            (left == flex_type_enum::DICT) ||
                            (left == flex_type_enum::LIST);
  } else if (op == "left_abs") {
    // right part of the operator is ignored in this one
    operation_is_feasible = (left == flex_type_enum::FLOAT
                             || left == flex_type_enum::INTEGER
                             || left == flex_type_enum::VECTOR);
  } else {
    log_and_throw("Invalid scalar operation");
  }

  if (!operation_is_feasible) {
    throw std::string("Unsupported type operation. cannot perform operation ") +
          op + " between " +
          flex_type_enum_to_name(left) + " and " +
          flex_type_enum_to_name(right);
  }
}

flex_type_enum get_output_type(flex_type_enum left,
                               flex_type_enum right,
                               std::string op) {
  if (op == "+" || op == "-" || op == "*") {
    if (left == flex_type_enum::INTEGER && right == flex_type_enum::FLOAT) {
      // operations against float always returns float
      return flex_type_enum::FLOAT;
    } else if (left == flex_type_enum::DATETIME && right == flex_type_enum::DATETIME && op == "-") {
      return flex_type_enum::FLOAT;
    } else if (left == flex_type_enum::ND_VECTOR || right == flex_type_enum::ND_VECTOR) {
      // vector operations always return vector
      return flex_type_enum::ND_VECTOR;
    } else if (left == flex_type_enum::VECTOR || right == flex_type_enum::VECTOR) {
      // vector operations always return vector
      return flex_type_enum::VECTOR;
    } else {
      // otherwise we take the type on the left hand side
      return left;
    }
  } else if (op == "**") {
    if (left == flex_type_enum::VECTOR || right == flex_type_enum::VECTOR) {
      // vector operations always return vector
      return flex_type_enum::VECTOR;
    } else {
      return flex_type_enum::FLOAT;
    }
  } else if (op == "//") {
    if (left == flex_type_enum::VECTOR || right == flex_type_enum::VECTOR) {
      // vector operations always return vector
      return flex_type_enum::VECTOR;
    } else if (left == flex_type_enum::FLOAT || right == flex_type_enum::FLOAT) {
      return flex_type_enum::FLOAT;
    } else {
      return flex_type_enum::INTEGER;
    }
  } else if (op == "%") {
    if (left == flex_type_enum::VECTOR || left == flex_type_enum::ND_VECTOR) {
      return left;
    }
    return flex_type_enum::INTEGER;
  } else if (op == "/") {
    if (left == flex_type_enum::ND_VECTOR || right == flex_type_enum::ND_VECTOR) {
      // vector operations always return vector
      return flex_type_enum::ND_VECTOR;
    } else if (left == flex_type_enum::VECTOR || right == flex_type_enum::VECTOR) {
      // vector operations always return vector
      return flex_type_enum::VECTOR;
    } else {
      return flex_type_enum::FLOAT;
    }
  } else if (op == "<" || op == ">" || op == "<=" ||
             op == ">=" || op == "==" || op == "!=") {
    // comparison always returns integer
    return flex_type_enum::INTEGER;
  } else if (op == "&" || op == "|") {
    // boolean operations always returns integer
    return flex_type_enum::INTEGER;
  } else if (op == "in") {
    return flex_type_enum::INTEGER;
  } else if (op == "left_abs") {
    return left;
  } else {
    throw std::string("Invalid Operation Type");
  }
}


std::function<flexible_type(const flexible_type&, const flexible_type&)>
get_binary_operator(flex_type_enum left, flex_type_enum right, std::string op) {
/**************************************************************************/
/*                                                                        */
/*                               Operator +                               */
/*                                                                        */
/**************************************************************************/
  if (op == "+") {
    if (left == flex_type_enum::INTEGER && right == flex_type_enum::FLOAT) {
      // integer + float is float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return (flex_float)l + (flex_float)r;
      };
    } else if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       if (l.size() != r.size()) return FLEX_UNDEFINED;
       return l + r;
     };
    } else if (left == flex_type_enum::VECTOR || left == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l + r; };
    } else if (right == flex_type_enum::VECTOR || right == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return r + l; };
    } else {
      // everything else, left hand side is good
      // int + int -> int
      // float + int -> float
      // float + float -> float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{return l + r;};
    }
/**************************************************************************/
/*                                                                        */
/*                               Operator -                               */
/*                                                                        */
/**************************************************************************/
  } else if (op == "-") {
    if (left == flex_type_enum::INTEGER && right == flex_type_enum::FLOAT) {
      // integer - float is float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return (flex_float)l - (flex_float)r;
      };
    } else if (left == flex_type_enum::DATETIME && right == flex_type_enum::DATETIME) {
      // integer - float is float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return l.get<flex_date_time>().microsecond_res_timestamp() -
               r.get<flex_date_time>().microsecond_res_timestamp();
      };
    } else if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       if (l.size() != r.size()) return FLEX_UNDEFINED;
       return l - r;
     };
    } else if (left == flex_type_enum::VECTOR || left == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l - r; };
    } else if (right == flex_type_enum::VECTOR || right == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return -r + l; };
    } else {
      // everything else, left hand side is good
      // int - int -> int
      // float - int -> float
      // float - float -> float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{return l - r;};
    }
/**************************************************************************/
/*                                                                        */
/*                               Operator *                               */
/*                                                                        */
/**************************************************************************/
  } else if (op == "*") {
    if (left == flex_type_enum::INTEGER && right == flex_type_enum::FLOAT) {
      // integer * float is float
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return (flex_float)l * (flex_float)r;
      };
    } else if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       if (l.size() != r.size()) return FLEX_UNDEFINED;
       return l * r;
     };
    } else if (left == flex_type_enum::ND_VECTOR && right == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l * r; };
    } else if (left == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l * r; };
    } else if (right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return r * l; };
    } else if (left == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l * r; };
    } else if (right == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return r * l; };
    } else {
      // everything else, left hand side is good
      // int * int -> int
      // float * int -> float
      // float * float -> float
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return l * r;};
    }
/**************************************************************************/
/*                                                                        */
/*                               Operator /                               */
/*                                                                        */
/**************************************************************************/
  } else if (op == "/") {

    if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       if (l.size() != r.size()) return FLEX_UNDEFINED;
       return l / r;
     };
    } else if (left == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l / r; };
    } else if (right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       flexible_type ret = r;
       for (size_t i = 0;i < ret.size(); ++i) ret[i] = l / ret[i];
       return ret;
     };
    } else if (left == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{ return l / r; };
    } else if (right == flex_type_enum::ND_VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       flexible_type ret = r;
       flex_nd_vec& v = ret.mutable_get<flex_nd_vec>();
       v.ensure_unique();
       double dl = l.to<double>();
       for (auto & i: v.elements()) {
         i = dl / i;
       }
       return ret;
     };
    } else {
      // divide always returns floats
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return (flex_float)l / (flex_float)r;
      };
    }
/**************************************************************************/
/*                                                                        */
/*                               Operator **                              */
/*                                                                        */
/**************************************************************************/
  } else if (op == "**") {

    if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       const flex_vec& lv = l.get<flex_vec>();
       const flex_vec& rv = r.get<flex_vec>();
       if (lv.size() != rv.size()) return FLEX_UNDEFINED;
       flex_vec ret(lv.size());
       for(size_t i = 0; i < lv.size(); ++i) {
         ret[i] = std::pow(lv[i], rv[i]);
       }
       return flexible_type(std::move(ret));
     };
    } else if (left == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       const flex_vec& lv = l.get<flex_vec>();
       flex_float rd = r.to<flex_float>();
       flex_vec ret(lv.size());
       for(size_t i = 0; i < lv.size(); ++i) {
         ret[i] = std::pow(lv[i], rd);
       }
       return flexible_type(std::move(ret));
     };
    } else if (right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       flex_float ld = l.to<flex_float>();
       const flex_vec& rv = r.get<flex_vec>();
       flex_vec ret(rv.size());
       for(size_t i = 0; i < rv.size(); ++i) {
         ret[i] = std::pow(ld, rv[i]);
       }
       return flexible_type(std::move(ret));
     };
    } else {
      // std::pow always returns floats
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        flex_float rd = (flex_float)r;
        if (rd == 0.5) {
          return std::sqrt((flex_float)l);
        } else {
          return std::pow((flex_float)l, (flex_float)r);
        }
      };
    }
  } else if (op == "//") {

    if (left == flex_type_enum::VECTOR && right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       const flex_vec& lv = l.get<flex_vec>();
       const flex_vec& rv = r.get<flex_vec>();
       if (lv.size() != rv.size()) return FLEX_UNDEFINED;
       flex_vec ret(lv.size());
       for(size_t i = 0; i < lv.size(); ++i) {
         flex_float res = lv[i] / rv[i];
         ret[i] = std::isfinite(res) ? std::floor(res) : res;
       }
       return flexible_type(std::move(ret));
     };
    } else if (left == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       const flex_vec& lv = l.get<flex_vec>();
       flex_float rd = r.to<flex_float>();
       flex_vec ret(lv.size());
       for(size_t i = 0; i < lv.size(); ++i) {
         flex_float res = lv[i] / rd;
         ret[i] = std::isfinite(res) ? std::floor(res) : res;
       }
       return flexible_type(std::move(ret));
     };
    } else if (right == flex_type_enum::VECTOR) {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       flex_float ld = l.to<flex_float>();
       const flex_vec& rv = r.get<flex_vec>();
       flex_vec ret(rv.size());
       for(size_t i = 0; i < rv.size(); ++i) {
         flex_float res = ld / rv[i];
         ret[i] = std::isfinite(res) ? std::floor(res) : res;
       }
       return flexible_type(std::move(ret));
     };
    } else if (right == flex_type_enum::FLOAT || left == flex_type_enum::FLOAT) {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type {
       flex_float ld = l.to<flex_float>();
       flex_float rd = r.to<flex_float>();
       flex_float res = ld / rd;

       return std::isfinite(res) ? std::floor(res) : res;
      };
    } else {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type {
       flex_float ld = l.to<flex_float>();
       flex_float rd = r.to<flex_float>();
       flex_float res = ld / rd;

       return (std::isfinite(res)
               ? flexible_type(flex_int(std::floor(res)))
               : FLEX_UNDEFINED);
      };
    }

/**************************************************************************/
/*                                                                        */
/*                               Operator %                               */
/*                                                                        */
/**************************************************************************/
  } else if (op == "%") {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type {
        if (l.get_type() == flex_type_enum::INTEGER &&
            r.get_type() == flex_type_enum::INTEGER) {
          flex_int rightval = r.get<flex_int>();
          flex_int leftval = l.get<flex_int>();
          /*
           * desired result
           * 1 % 3 == 1
           * -1 % 3 == 2
           * 1 % -3 == -2
           * -1 % -3 == -1
           */
          if (rightval > 0) {
            // Result of remainder operator is implementation dependent. It
            // could be negative or positive. We lock it to positive.
            // That makes more sense.
            auto res = leftval % rightval;
            if (res < 0) res += rightval;
            return res;
          } else if (rightval < 0) {
            // if the rightval is negative that is also implementation
            // dependent. We lock it to negative. This matches Python's
            // definition
            auto res = leftval % (-rightval);
            if (res > 0) res += rightval; // remember rightval is negative here
            return res;
          } else {
            return FLEX_UNDEFINED;
          }
        } else {
          return 0;
        }
      };
/**************************************************************************/
/*                                                                        */
/*                              Operator in                               */
/*                                                                        */
/**************************************************************************/
    } else if (op == "in") {
      // note that while the op is "in", the direction of the operator is
      // [BIGGER_LIST "in" element] rather than ["element" in BIGGER_LIST]
      // yes. slightly disconcerting I know. Sorry.
      if (left == flex_type_enum::STRING && right == flex_type_enum::STRING) {
        return [](const flexible_type& l, const flexible_type& r)->flexible_type {
          if (l.get_type() == flex_type_enum::STRING &&
              r.get_type() == flex_type_enum::STRING ) {
            const auto& left_str = l.get<flex_string>();
            const auto& right_str = r.get<flex_string>();
            return left_str.find(right_str) != std::string::npos;
          } else {
            return 0;
          }
        };
      } else if (left == flex_type_enum::VECTOR &&
                 (right == flex_type_enum::FLOAT || right == flex_type_enum::INTEGER)) {
        return [](const flexible_type& l, const flexible_type& r)->flexible_type {
          if (l.get_type() == flex_type_enum::VECTOR &&
              (r.get_type() == flex_type_enum::FLOAT || r.get_type() == flex_type_enum::INTEGER)) {
            const auto& vec = l.get<flex_vec>();
            const auto val = (flex_float)r;
            int ret = (std::find(vec.begin(), vec.end(), val) != vec.end());
            return ret;
          } else {
            return 0;
          }
        };
      } else if (left == flex_type_enum::LIST) {
        return [](const flexible_type& l, const flexible_type& r)->flexible_type {
          if (l.get_type() == flex_type_enum::LIST) {
            const auto& vec = l.get<flex_list>();
            int ret = std::find(vec.begin(), vec.end(), r) != vec.end();
            return ret;
          } else {
            return 0;
          }
        };
      } else if (left == flex_type_enum::DICT) {
        return [](const flexible_type& l, const flexible_type& r)->flexible_type {
          if (l.get_type() == flex_type_enum::DICT) {
            const auto& dict = l.get<flex_dict>();
            for (const auto& elem: dict) {
              if (elem.first == r) return 1;
            }
            return 0;
          } else {
            return 0;
          }
        };
      } else {
        throw std::string("Invalid operands for flexible_type binary operator");
      }
/**************************************************************************/
/*                                                                        */
/*                      abs of the left value                             */
/*                                                                        */
/**************************************************************************/
  } else if (op == "left_abs") {
    // ** always returns floats
    if(left == flex_type_enum::VECTOR) {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        const flex_vec& v = l.get<flex_vec>();
        flex_vec ret(v.size());
        for(size_t i = 0; i < v.size(); ++i) {
          ret[i] = std::abs(v[i]);
        }
        return flexible_type(std::move(ret));
      };
    } else if(left == flex_type_enum::INTEGER) {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return std::abs(l.get<flex_int>());
      };
    } else {
      return [](const flexible_type& l, const flexible_type& r)->flexible_type{
        return std::abs(l.to<flex_float>());
      };
    }

/**************************************************************************/
/*                                                                        */
/*                          Comparison Operators                          */
/*                                                                        */
/**************************************************************************/
  } else if (op == "<") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l < r);};
  } else if (op == ">") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l > r);};
  } else if (op == "<=") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l <= r);};
  } else if (op == ">=") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l >= r);};
  } else if (op == "==") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l == r);};
  } else if (op == "!=") {
     return [](const flexible_type& l, const flexible_type& r)->flexible_type{return (int)(l != r);};
  } else if (op == "&") {
    // Ternary truth table
    //  & | F - T
    //  -------
    //  F | F F F
    //  - | F - -
    //  T | F - T
    //
    return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       int ldef = !l.is_na();
       int rdef = !r.is_na();
       bool lval = !l.is_zero(); // note that NA is 0
       bool rval = !r.is_zero();
       if (ldef && rdef) return int(lval && rval); // regular case. Both are defined
       else if (!ldef) return (rdef && rval == false) ? flexible_type(0) : FLEX_UNDEFINED;
       else /* if (!rdef) */ return (ldef && lval == false) ? flexible_type(0) : FLEX_UNDEFINED;
     };
  } else if (op == "|") {
    // Ternary truth table
    //  | | F - T
    //  -------
    //  F | F - T
    //  - | - - T
    //  T | T T T
    //
    return [](const flexible_type& l, const flexible_type& r)->flexible_type{
       int ldef = !l.is_na();
       int rdef = !r.is_na();
       bool lval = !l.is_zero(); // note that NA is 0
       bool rval = !r.is_zero();
       if (ldef && rdef) return int(lval || rval); // regular case. Both are defined
       else if (!ldef) return (rdef && rval == true) ? flexible_type(1) : FLEX_UNDEFINED;
       else /* if (!rdef) */ return (ldef && lval == true) ? flexible_type(1) : FLEX_UNDEFINED;
     };
  } else {
    throw std::string("Invalid Operation Type");
  }
}



} // namespace unity_sarray_binary_operations
} // namespace turi
