/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/toolkit_class_base.hpp>

namespace turi {
toolkit_class_base::~toolkit_class_base() { }

std::vector<std::string> toolkit_class_base::list_keys() {
  return {"list_functions", 
    "call_function", 
    "list_get_properties",
    "list_set_properties",
    "set_property", 
    "get_property",
    "get_docstring",
    "__name__",
    "__uid__"};
}

variant_type toolkit_class_base::get_value(std::string key, variant_map_type& arg) {
  perform_registration();
  if (key == "list_functions") {
    return to_variant(list_functions());
  } else if (key == "list_get_properties") {
    return to_variant(list_get_properties());
  } else if (key == "list_set_properties") {
    return to_variant(list_set_properties());
  } else if (key == "call_function") {
    // dispatches to a user defined function
    if (!arg.count("__function_name__")) {
      throw("Invalid function call format");
    }
    std::string function_name = variant_get_value<std::string>(arg["__function_name__"]);
    return to_variant(call_function(function_name, arg));
  } else if (key == "set_property") {
    // dispatches to a user defined set property
    if (!arg.count("__property_name__")) {
      throw("Invalid function call format");
    }
    std::string property_name = variant_get_value<std::string>(arg["__property_name__"]);
    return to_variant(set_property(property_name, arg));
  } else if (key == "get_property") {
    // dispatches to a user defined get property
    if (!arg.count("__property_name__")) {
      throw("Invalid function call format");
    }
    std::string property_name = variant_get_value<std::string>(arg["__property_name__"]);
    return to_variant(get_property(property_name, arg));
  } else if (key == "get_docstring") {
    // dispatches to a user defined get property
    if (!arg.count("__symbol__")) {
      throw("Invalid function call format");
    }
    std::string symbol = variant_get_value<std::string>(arg["__symbol__"]);
    return to_variant(get_docstring(symbol));
  } else if (key == "__name__") {
    return name();
  } else if (key == "__uid__") {
    return uid();
  } else {
    return variant_type();
  }
}

void toolkit_class_base::save_impl(oarchive& oarc) const { }

void toolkit_class_base::load_version(iarchive& iarc, size_t version) { }

size_t toolkit_class_base::get_version() const {
  return 0;
}

std::map<std::string, std::vector<std::string>> toolkit_class_base::list_functions() {
  perform_registration();
  return m_function_args;
}

std::vector<std::string> toolkit_class_base::list_get_properties() {
  perform_registration();
  std::vector<std::string> ret;
  for(const auto& i: m_get_property_list) ret.push_back(i.first);
  return ret;
  return std::vector<std::string>();
}

std::vector<std::string> toolkit_class_base::list_set_properties() {
  perform_registration();
  std::vector<std::string> ret;
  for(const auto& i: m_set_property_list) ret.push_back(i.first);
  return ret;
}

variant_type toolkit_class_base::call_function(std::string function, 
                                               variant_map_type argument) {
  perform_registration();
  if (m_function_list.count(function)) {
    return m_function_list[function](this, argument);
  } else {
    throw(std::string("No such property"));
  }
}

variant_type toolkit_class_base::get_property(std::string property,
                                                      variant_map_type argument) {
  perform_registration();
  if (m_get_property_list.count(property)) {
    return m_get_property_list[property](this, argument);
  } else {
    throw(std::string("No such property"));
  }
}

/**
 * Sets a property.
 */ 
variant_type toolkit_class_base::set_property(std::string property, 
                                                      variant_map_type argument) {
  perform_registration();
  if (m_set_property_list.count(property)) {
    return m_set_property_list[property](this, argument);
  } else {
    throw(std::string("No such property"));
  }
}

std::string toolkit_class_base::get_docstring(std::string symbol) {
  if (m_docstring.count(symbol)) {
    return m_docstring.at(symbol);
  } else {
    return "";
  }
}


void toolkit_class_base::register_function(std::string fnname, 
                                           std::vector<std::string> arguments,
                                           std::function<variant_type(toolkit_class_base*, variant_map_type)> fn) {

  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) fnname = fnname.substr(last_colon + 1);

  m_function_args[fnname] = arguments;
  m_function_list[fnname] = fn;
}

/**
 * Adds a property setter with the specified name.
 */
void toolkit_class_base::register_setter(std::string propname, 
                                         std::function<variant_type(toolkit_class_base*, variant_map_type)> setfn) {
  m_set_property_list[propname] = setfn;
}

/**
 * Adds a property getter with the specified name.
 */
void toolkit_class_base::register_getter(std::string propname, 
                                         std::function<variant_type(toolkit_class_base*, variant_map_type)> getfn) {
  m_get_property_list[propname] = getfn;
}

void toolkit_class_base::register_docstring(std::pair<std::string, std::string> fnname_docstring) {
  std::string fnname;
  std::string docstring;
  std::tie(fnname, docstring) = fnname_docstring;
  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) fnname = fnname.substr(last_colon + 1);
  m_docstring[fnname] = docstring;
}

} // namespace turi
