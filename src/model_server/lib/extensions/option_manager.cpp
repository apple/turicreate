/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/extensions/option_manager.hpp>

#include <boost/algorithm/string.hpp>

namespace turi {


void option_manager::create_real_option(const std::string& name,
                                        const std::string& description,
                                        flexible_type default_value,
                                        double lower_bound,
                                        double upper_bound,
                                        bool allowed_overwrite) {
  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::REAL;

  opt.name = name;
  opt.description = description;
  opt.default_value = (flexible_type)default_value;
  opt.lower_bound = (double)lower_bound;
  opt.upper_bound = (double)upper_bound;

  create_option(opt, allowed_overwrite);
}

void option_manager::create_integer_option(const std::string& name,
                                           const std::string& description,
                                           flexible_type default_value,
                                           flex_int lower_bound,
                                           flex_int upper_bound,
                                           bool allowed_overwrite) {
  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::INTEGER;

  opt.name = name;
  opt.description = description;
  opt.default_value = default_value;
  opt.lower_bound = lower_bound;
  opt.upper_bound = upper_bound;

  create_option(opt, allowed_overwrite);
}

void option_manager::create_categorical_option(const std::string& name,
                                               const std::string& description,
                                               const flexible_type& default_value,
                                               const std::vector<flexible_type>& allowed_values,
                                               bool allowed_overwrite) {
  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::CATEGORICAL;

  opt.name = name;
  opt.description = description;
  opt.default_value = default_value;
  opt.allowed_values = allowed_values;

  create_option(opt, allowed_overwrite);
}

void option_manager::create_string_option(const std::string& name,
                                          const std::string& description,
                                          const flexible_type& default_value,
                                          bool allowed_overwrite) {

  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::STRING;

  opt.name = name;
  opt.description = description;
  opt.default_value = default_value;

  create_option(opt, allowed_overwrite);
}

void option_manager::create_boolean_option(const std::string& name,
                                           const std::string& description,
                                           bool default_value,
                                           bool allowed_overwrite) {
  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::BOOL;

  opt.name = name;
  opt.description = description;
  opt.default_value = default_value;

  create_option(opt, allowed_overwrite);
}

void option_manager::create_flexible_type_option(const std::string& name,
                                                 const std::string& description,
                                                 const flexible_type& default_value,
                                                 bool allowed_overwrite) {

  option_handling::option_info opt;

  opt.parameter_type = option_handling::option_info::FLEXIBLE_TYPE;

  opt.name = name;
  opt.description = description;
  opt.default_value = default_value;

  create_option(opt, allowed_overwrite);
}


void option_manager::create_option(const option_handling::option_info& opt, bool allow_override) {

  auto it = options_reference_lookup_map.find(opt.name);

  if(it != options_reference_lookup_map.end()) {

    DASSERT_LT(it->second, options_reference.size());

    if(allow_override) {
      size_t idx = it->second;
      options_reference[idx] = opt;
      _current_option_values[opt.name] = opt.default_value;
    } else {
      logstream(LOG_DEBUG) << "Option " << opt.name << " defined a second time.";
    }
  } else {
    options_reference.push_back(opt);
    options_reference_lookup_map[opt.name] = options_reference.size() - 1;
    _current_option_values[opt.name] = opt.default_value;
  }
}


/** Set one of the options.  This values is checked against the
 *  requirements given by the option instance.
 */
void option_manager::set_option(const std::string& name, const flexible_type& value) {

  // Ignore internal options
  if (boost::starts_with(name, "_")) {
    logstream(LOG_INFO) << "Ignore internal option " << name
                        << ": " << value << std::endl;
    return;
  }

  auto it = options_reference_lookup_map.find(name);

  if(it == options_reference_lookup_map.end()) {
    log_and_throw(std::string("Option '") + name + "' not recognized.");
  }

  DASSERT_LT(it->second, options_reference.size());

  _current_option_values[name] = options_reference[it->second].interpret_value(value);
}

/** Sets the options.  These values are checked against the values
 *  in the option reference.
 */
void option_manager::set_options(const std::map<std::string, flexible_type>& options) {
  for(const auto& v : options) {
    set_option(v.first, v.second);
  }
}

/** Delete one of the options. This removes the option from
 *  options_reference_lookup_map and _current_option_values,
 *  but does not remove it from the vector options_reference.
 *  Useful for loading from older model versions with obsolete option names.
 */
void option_manager::delete_option(const std::string& name) {

  auto it = options_reference_lookup_map.find(name);

  if (it != options_reference_lookup_map.end()) {
    options_reference_lookup_map.erase(it);
    _current_option_values.erase(name);
  }
}

/** Delete a set of options. This removes the options from
 *  options_reference_lookup_map and _current_option_values,
 *  but does not remove them from the vector options_reference.
 *  Useful for loading from older model versions with obsolete option names.
 */
void option_manager::delete_options(const std::vector<std::string>& names) {
  for (const auto& v : names)
    delete_option(v);
}

/** Update the name of an option. Useful for loading from older model versions.
 *  If an option exists with old_name, create a new option with new_name,
 *  copy over values from the old_name, then remove the old option.
 */
void option_manager::update_option_name(const std::string& old_name, const std::string& new_name) {

  auto it = options_reference_lookup_map.find(old_name);

  if (it == options_reference_lookup_map.end())
    return;

  // Create a new option with a different name from the old option,
  // but the same attributes and value.
  const option_handling::option_info& old_option = options_reference[it->second];
  option_handling::option_info new_option;
  new_option.name = new_name;
  new_option.description = old_option.description;
  new_option.default_value = old_option.default_value;
  new_option.parameter_type = old_option.parameter_type;

  create_option(new_option, false);

  set_option(new_name, value(old_name));

  // Delete the old option
  delete_option(old_name);
}

/** Update the name of a set of options.
 *  Useful for loading from older model versions.
 *  If an option exists with old_name, create a new option with new_name,
 *  copy over values from the old_name, then remove the old option.
 */
void option_manager::update_option_names(const std::map<std::string, std::string>& name_map) {
  for (const auto& v : name_map)
    update_option_name(v.first, v.second);
}

/// Returns the option information struct for each of the set parameters.
const std::vector<option_handling::option_info>& option_manager::get_option_info() const {
  return options_reference;
}

/// Returns a map of strings to flexible_type that give the values
/// of all the current option values.
const std::map<std::string, flexible_type>& option_manager::current_option_values() const {
  return _current_option_values;
}


/** Creates and returns a map of the default options, as specified
 * by the model.
 */
std::map<std::string, flexible_type> option_manager::get_default_options() const {

  std::map<std::string, flexible_type> ret;

  for(const auto& opt_info : options_reference)
    ret[opt_info.name] = opt_info.default_value;

  return ret;
}


const flexible_type& option_manager::value(const std::string& field) const {

  auto it = _current_option_values.find(field);

  if(it == _current_option_values.end())
    log_and_throw(std::string("Option '") + field + "' does not exist.");

  return it->second;
}


/// Returns true if an option exists and false otherwise
bool option_manager::is_option(const std::string& name) const {
  return options_reference_lookup_map.count(name) != 0;
}

/// Returns true if an option exists and false otherwise
const std::string& option_manager::description(const std::string& name) const {
  auto it = options_reference_lookup_map.find(name);

  if(it == options_reference_lookup_map.end())
    log_and_throw(std::string("Option '") + name + "' does not exist.");

  return options_reference[it->second].description;
}

  /// Serialization -- save
void option_manager::save(turi::oarchive& oarc) const {
  oarc << options_reference_lookup_map << options_reference << _current_option_values;
}

  /// Serialization -- load
void option_manager::load(turi::iarchive& iarc) {
  iarc >> options_reference_lookup_map >> options_reference >> _current_option_values;
}

}
