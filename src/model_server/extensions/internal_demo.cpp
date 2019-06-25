/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/extensions/model_base.hpp>

using namespace turi;

int _demo_addone(int param) {
  return param + 1;
}

variant_type _demo_identity(variant_type input) {
  // this is used in unit tests
  return input;
}

double _demo_add(double param, double param2) {
  return param + param2;
}

std::string _demo_to_string(flexible_type param) {
  return std::string(param);
}


std::string _replicate(const std::map<std::string, flexible_type>& input) {
  std::string ret;
  size_t reptimes = input.at("reptimes");
  std::string value = input.at("value");
  for (size_t i = 0;i < reptimes; ++i) {
    ret = ret + value;
  }
  return ret;
}


flexible_type _replicate_column(const std::map<std::string, flexible_type>& input,
                                std::string column,
                                size_t times) {
  flexible_type val = input.at(column);
  if (times == 0) return FLEX_UNDEFINED;
  flexible_type ret = val;
  for (size_t i = 1;i < times; ++i) {
    ret = ret + val;
  }
  return ret;
}

std::vector<variant_type> _connected_components(std::map<std::string, flexible_type>& src,
                                               std::map<std::string, flexible_type>& edge,
                                               std::map<std::string, flexible_type>& dst) {
  if (src["cc"] < dst["cc"]) dst["cc"] = src["cc"];
  else src["cc"] = dst["cc"];
  return {to_variant(src), to_variant(edge), to_variant(dst)};
}



std::vector<variant_type> _connected_components_parameterized(std::map<std::string, flexible_type>& src,
                                                             std::map<std::string, flexible_type>& edge,
                                                             std::map<std::string, flexible_type>& dst,
                                                             std::string column) {
  if (src[column] < dst[column]) dst[column] = src[column];
  else src[column] = dst[column];
  return {to_variant(src), to_variant(edge), to_variant(dst)};
}




class demo_class: public model_base {
  virtual void save_impl(oarchive& oarc) const override {}

  virtual void load_version(iarchive& iarc, size_t version) override {}

  // Simple Functions
  std::string concat() {
    return one + two;
  }

  // Simple Functions
  std::string concat_more(std::string three) {
    return one + two + three;
  }
  // simple properties
  std::string one;
  std::string two;

  // getter/setter properties
  std::string two_getter() const {
    return two;
  }
  void two_setter(std::string param) {
    two = param + " pika";
  }

  BEGIN_CLASS_MEMBER_REGISTRATION("demo_class")
  REGISTER_CLASS_DOCSTRING("A simple demo class which concats strings.")
  REGISTER_CLASS_MEMBER_FUNCTION(demo_class::concat);
  REGISTER_CLASS_MEMBER_FUNCTION(demo_class::concat_more, "three");

  REGISTER_PROPERTY(one);

  REGISTER_GETTER("two", demo_class::two_getter);
  REGISTER_SETTER("two", demo_class::two_setter);

  REGISTER_CLASS_MEMBER_DOCSTRING(demo_class::concat, "Concatenates the values one and two")
  REGISTER_CLASS_MEMBER_DOCSTRING("concat_more",
                                  "Concatenates the values one and two and the argument three")
  END_CLASS_MEMBER_REGISTRATION
};

class demo_vector : public std::vector<std::string>, public model_base {
 public:
  virtual void save_impl(oarchive& oarc) const override {}

  virtual void load_version(iarchive& iarc, size_t version) override {}

  std::string __str__() {
    std::stringstream strm;
    strm << "[";
    for (size_t i = 0; i < size(); ++i) {
      strm << at(i) << ", ";
    }
    strm << "]";
    return strm.str();
  }

  void push_back_impl(const flexible_type& i) {
    return push_back((flex_string)i);
  }

  std::string __getitem__(size_t i) const {
    return at(i);
  }

  void __setitem__(size_t i, const flexible_type& val) {
    at(i) = (flex_string)(val);
  }

  void __delitem__(size_t i) {
    if (i < size()) {
      erase(begin() + i);
    }
  }
  BEGIN_CLASS_MEMBER_REGISTRATION("demo_vector")

  // typedef void (demo_vector::*push_back_type)(const std::string&);
  // REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION(push_back_type, demo_vector::push_back, "param");

  // typedef std::string& (demo_vector::*at_type)(size_t);
  // REGISTER_OVERLOADED_NAMED_CLASS_MEMBER_FUNCTION("__getitem__", at_type, demo_vector::at, "param");

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("__len__", demo_vector::size);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("len", demo_vector::size);
  REGISTER_CLASS_MEMBER_FUNCTION(demo_vector::__str__);
  REGISTER_CLASS_MEMBER_FUNCTION(demo_vector::__getitem__, "idx");
  REGISTER_CLASS_MEMBER_FUNCTION(demo_vector::__setitem__, "idx", "value");
  REGISTER_CLASS_MEMBER_FUNCTION(demo_vector::__delitem__, "idx");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("__repr__", demo_vector::__str__);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("push_back", demo_vector::push_back_impl, "param");
  END_CLASS_MEMBER_REGISTRATION
};

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_demo_addone, "param");
REGISTER_FUNCTION(_demo_identity, "param");
REGISTER_FUNCTION(_demo_add, "param", "param2");
REGISTER_FUNCTION(_demo_to_string, "param");
REGISTER_FUNCTION(_replicate, "input");
REGISTER_FUNCTION(_replicate_column, "input", "column", "times");
REGISTER_FUNCTION(_connected_components, "src", "edge", "dst");
REGISTER_FUNCTION(_connected_components_parameterized, "src", "edge", "dst", "column");
REGISTER_DOCSTRING(_demo_add, "Adds")
END_FUNCTION_REGISTRATION

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(demo_class)
REGISTER_CLASS(demo_vector)
END_CLASS_REGISTRATION
