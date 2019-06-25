/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/extensions/model_base.hpp>
#include <model_server/lib/unity_global.hpp>

namespace turi {

model_base::~model_base() = default;

const std::map<std::string, std::vector<std::string> >&
model_base::list_functions() {
  _check_registration();
  return m_function_args;
}

const std::vector<std::string>& model_base::list_get_properties() {
  _check_registration();

  if (m_get_property_cache.empty()) {
    m_get_property_cache.reserve(m_get_property_list.size());
    for (const auto& i : m_get_property_list) {
      m_get_property_cache.push_back(i.first);
    }
}

  return m_get_property_cache;
}

const std::vector<std::string>& model_base::list_set_properties() {
  _check_registration();

  if (m_set_property_cache.empty()) {
    m_set_property_cache.reserve(m_set_property_list.size());
    for (const auto& i : m_set_property_list) {
      m_set_property_cache.push_back(i.first);
    }
  }

  return m_set_property_cache;
}

inline void model_base::_check_registration() {
  if (!is_registered()) {
  perform_registration();
}
}

template <typename T>
void model_base::_raise_not_found_error(const std::string& n,
                                        const std::map<std::string, T>& m) {
  DASSERT_EQ(m.count(n), 0);

  std::ostringstream ss;

  ss << "Method/property " << n << " not registered for class " << name()
     << "; possible values are: ";

  size_t c = m.size();
  for (const auto& p : m) {
    ss << p.first << ((--c) != 0 ? ", " : ".");
  }

  log_and_throw(ss.str());
}

std::string model_base::_make_method_name(const std::string& function) {
  const std::vector<std::string>& arguments = m_function_args.at(function);

  std::ostringstream ss;

  ss << function << "(";

  // Create a counter so we can put commas in the right places.
  size_t c = arguments.size();
  for (const std::string& a : arguments) {
    ss << a << (--c != 0 ? ", " : "");
  }

  ss << ")";

  return ss.str();
}

variant_type model_base::call_function(const std::string& function,
				       variant_map_type argument) {
  _check_registration();

  auto it = m_function_args.find(function);

  if (it == m_function_args.end()) {
    _raise_not_found_error(function, m_function_args);
  }

  const auto& function_args = it->second;

  // Check for missing arguments and fill in default args as needed.
  auto default_arg_it = m_function_default_args.find(function);

  std::vector<std::string> missing_args;
  size_t correct_count = 0;
  for (const std::string& a : function_args) {
    auto it = argument.find(a);
    if (it == argument.end()) {
      if (default_arg_it != m_function_default_args.end()) {
        const auto& default_arg_lookup = default_arg_it->second;

        auto d_it = default_arg_lookup.find(a);
        if (d_it != default_arg_lookup.end()) {
          argument[a] = d_it->second;
          ++correct_count;
          continue;
      }
    }
      // The default argument case didn't catch this.
      missing_args.push_back(a);
  } else {
      ++correct_count;
  }
}

  if (!missing_args.empty()) {
    std::ostringstream ss;
    ss << "Error: missing arguments for method " << _make_method_name(function)
       << " in model " << name() << ": ";

    for (const auto& a : missing_args) {
      ss << a << " ";
  }

    std_log_and_throw(std::invalid_argument, ss.str());
  }

  if (correct_count != argument.size()) {
    std::vector<std::string> extra_arg_list;
    std::set<std::string> function_arg_set(function_args.begin(),
                                           function_args.end());
    for (const auto& p : argument) {
      if (function_arg_set.count(p.first) == 0) {
        extra_arg_list.push_back(p.first);
      }
    }

    std::ostringstream ss;
    ss << "Error: extra parameters given for method " <<
        _make_method_name(function) << " in model " << name() << ": ";

    for (const auto& a : extra_arg_list) {
      ss << a << " ";
    }

    std::cerr << "WARNING: " << ss.str() << std::endl;
    // TODO: fix all warnings and make these errors.
    // std_log_and_throw(std::invalid_argument, ss.str());
  }

  return m_function_list.at(function)(this, std::move(argument));
}

variant_type model_base::get_property(const std::string& property) {
  _check_registration();

  auto it = m_get_property_list.find(property);

  if (it == m_get_property_list.end()) {
    _raise_not_found_error(property, m_get_property_list);
  }

  return it->second(this, variant_map_type());
}

/**
 * Sets a property.
 */
variant_type model_base::set_property(const std::string& property,
				      variant_map_type argument) {
  _check_registration();

  auto it = m_set_property_list.find(property);

  if (it == m_set_property_list.end()) {
    _raise_not_found_error(property, m_set_property_list);
  }

  return it->second(this, variant_map_type());
}

const std::string& model_base::get_docstring(const std::string& symbol) {
  static std::string empty_str;
  auto it = m_docstring.find(symbol);
  if (it != m_docstring.end()) {
    return it->second;
  } else {
    return empty_str;
  }
}


void model_base::register_function(std::string fnname,
                                   const std::vector<std::string>& arguments,
				   impl_fn fn) {

  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) {
    fnname = fnname.substr(last_colon + 1);
  }

  m_function_args[fnname] = arguments;
  m_function_list[fnname] = std::move(fn);
}

void model_base::register_defaults(const std::string& fnname,
				   const variant_map_type& arguments) {
  m_function_default_args[fnname] = arguments;
}

/**
 * Adds a property setter with the specified name.
 */
void model_base::register_setter(const std::string& propname, impl_fn setfn) {
  m_set_property_list[propname] = std::move(setfn);
}

/**
 * Adds a property getter with the specified name.
 */
void model_base::register_getter(const std::string& propname, impl_fn getfn) {
  m_get_property_list[propname] = std::move(getfn);
}

void model_base::register_docstring(
    const std::pair<std::string, std::string>& fnname_docstring) {
  std::string fnname;
  std::string docstring;
  std::tie(fnname, docstring) = fnname_docstring;
  auto last_colon = fnname.find_last_of(":");
  if (last_colon != std::string::npos) fnname = fnname.substr(last_colon + 1);
  m_docstring[fnname] = docstring;
}

/**
 * Save a toolkit class to disk.
 *
 * \param sidedata Any additional side information
 * \param url The destination url to store the class.
 */
void model_base::save_to_url(const std::string& url,
                             const variant_map_type& side_data) {
  std::shared_ptr<model_base> m =
      std::dynamic_pointer_cast<model_base>(this->shared_from_this());

  turi::get_unity_global_singleton()->save_model(m, side_data, url);
}

/**
 * Save a toolkit class to a data stream.
 */
void model_base::save_model_to_data(std::ostream& out) {
  std::shared_ptr<model_base> m =
      std::dynamic_pointer_cast<model_base>(this->shared_from_this());

  turi::get_unity_global_singleton()->save_model_to_data(m, out);
}

void model_base::perform_registration() {
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("save", model_base::save_to_url, "url",
                                       "side_data");
  register_defaults("save", {{"side_data", variant_map_type()}});

  set_registered();
}

} // namespace turi
