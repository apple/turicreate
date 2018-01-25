/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/extensions/model_base.hpp>

namespace turi {

model_base::~model_base() = default;

void model_base::save_impl(oarchive& oarc) const {}

void model_base::load_version(iarchive& iarc, size_t version) {}

size_t model_base::get_version() const {
  return 0;
}

std::map<std::string, std::vector<std::string>> model_base::list_functions() {
  perform_registration();
  return m_function_args;
}

std::vector<std::string> model_base::list_get_properties() {
  perform_registration();
  std::vector<std::string> ret;
  for(const auto& i: m_get_property_list) ret.push_back(i.first);
  return ret;
}

std::vector<std::string> model_base::list_set_properties() {
  perform_registration();
  std::vector<std::string> ret;
  for(const auto& i: m_set_property_list) ret.push_back(i.first);
  return ret;
}

variant_type model_base::call_function(std::string function, 
				       variant_map_type argument) {
  perform_registration();
  if (m_function_list.count(function)) {
    // fill in default args if any
    if (m_function_default_args.count(function)) {
      for (const auto& p : m_function_default_args[function]) {
        if (!argument.count(p.first)) argument[p.first] = p.second;
      }
    }
    return m_function_list[function](this, argument);
  } else {
    throw(std::string("No such property"));
  }
}

variant_type model_base::get_property(std::string property) {
  perform_registration();
  if (m_get_property_list.count(property)) {
    return m_get_property_list[property](this, variant_map_type());
  } else {
    throw(std::string("No such property"));
  }
}

/**
 * Sets a property.
 */ 
variant_type model_base::set_property(std::string property, 
				      variant_map_type argument) {
  perform_registration();
  if (m_set_property_list.count(property)) {
    return m_set_property_list[property](this, argument);
  } else {
    throw(std::string("No such property"));
  }
}

std::string model_base::get_docstring(std::string symbol) {
  if (m_docstring.count(symbol)) {
    return m_docstring.at(symbol);
  } else {
    return "";
  }
}


void model_base::register_function(std::string fnname, 
				   std::vector<std::string> arguments,
				   impl_fn fn) {

  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) fnname = fnname.substr(last_colon + 1);

  m_function_args[fnname] = arguments;
  m_function_list[fnname] = std::move(fn);
}

void model_base::register_defaults(std::string fnname, 
				   const variant_map_type& arguments) {
  m_function_default_args[fnname] = arguments;
}

/**
 * Adds a property setter with the specified name.
 */
void model_base::register_setter(std::string propname, impl_fn setfn) {
  m_set_property_list[propname] = std::move(setfn);
}

/**
 * Adds a property getter with the specified name.
 */
void model_base::register_getter(std::string propname, impl_fn getfn) {
  m_get_property_list[propname] = std::move(getfn);
}

void model_base::register_docstring(
    std::pair<std::string, std::string> fnname_docstring) {
  std::string fnname;
  std::string docstring;
  std::tie(fnname, docstring) = fnname_docstring;
  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) fnname = fnname.substr(last_colon + 1);
  m_docstring[fnname] = docstring;
}

} // namespace turi
